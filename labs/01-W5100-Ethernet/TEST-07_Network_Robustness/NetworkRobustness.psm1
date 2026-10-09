# Shared TEST-07 transport and verdict functions. PowerShell 7+, no dependencies.
Set-StrictMode -Version Latest

function ConvertFrom-NetworkLine {
    param([string]$Line)
    if ($Line -notmatch '^(EVT|STAT|INFO) ') { return $null }
    $fields = [ordered]@{ type = $Matches[1] }
    foreach ($m in [regex]::Matches($Line, '(\w+)=([^\s]+)')) {
        $fields[$m.Groups[1].Value] = $m.Groups[2].Value
    }
    return [pscustomobject]$fields
}

function New-ProbePayload {
    param([int]$Length, [uint32]$Sequence, [byte[]]$Session)
    [byte[]]$data = [byte[]]::new($Length)
    for ($i = 0; $i -lt $Length; $i++) { $data[$i] = [byte](($i * 37 + $Sequence % 256) % 256) }
    if ($Length -ge 16) {
        [Array]::Copy($Session, 0, $data, 0, 8)
        [Array]::Copy([BitConverter]::GetBytes($Sequence), 0, $data, 8, 4)
    }
    return ,$data
}

function Test-UdpProbe {
    param([string]$IP, [int]$Port, [byte[]]$Data, [int]$TimeoutMs)
    $timer = [Diagnostics.Stopwatch]::StartNew()
    $udp = [Net.Sockets.UdpClient]::new(0)
    $code = 'PASS'; $detail = ''
    try {
        $udp.Client.ReceiveTimeout = $TimeoutMs
        $udp.Client.SendTimeout = $TimeoutMs
        $target = [Net.IPEndPoint]::new([Net.IPAddress]::Parse($IP), $Port)
        if ($udp.Send($Data, $Data.Length, $target) -ne $Data.Length) { throw 'SEND_SIZE' }
        $peer = [Net.IPEndPoint]::new([Net.IPAddress]::Any, 0)
        [byte[]]$reply = $udp.Receive([ref]$peer)
        if (-not $peer.Address.Equals($target.Address) -or $peer.Port -ne $Port) { $code = 'SOURCE' }
        elseif ($reply.Length -ne $Data.Length) { $code = 'LENGTH' }
        else {
            for ($j = 0; $j -lt $Data.Length; $j++) {
                if ($Data[$j] -ne $reply[$j]) { $code = 'CORRUPT'; $detail = "offset=$j"; break }
            }
        }
    } catch [Net.Sockets.SocketException] {
        $code = if ($_.Exception.SocketErrorCode -eq [Net.Sockets.SocketError]::TimedOut) { 'TIMEOUT' } else { 'SOCKET' }
        $detail = $_.Exception.SocketErrorCode.ToString()
    } catch { $code = 'ERROR'; $detail = $_.Exception.Message }
    finally { $udp.Dispose() }
    [pscustomobject]@{ Code = $code; Detail = $detail; Ms = $timer.Elapsed.TotalMilliseconds }
}

function Test-TcpProbe {
    param([string]$IP, [int]$Port, [string]$Payload, [int]$TimeoutMs, [switch]$Abort)
    $timer = [Diagnostics.Stopwatch]::StartNew()
    $tcp = [Net.Sockets.TcpClient]::new()
    $code = 'PASS'; $detail = ''
    try {
        $task = $tcp.ConnectAsync([Net.IPAddress]::Parse($IP), $Port)
        if (-not $task.Wait($TimeoutMs)) { throw 'TIMEOUT' }
        $stream = $tcp.GetStream()
        $remaining = [Math]::Max(1, $TimeoutMs - [int]$timer.ElapsedMilliseconds)
        $stream.WriteTimeout = $remaining
        $request = [Text.Encoding]::ASCII.GetBytes($Payload + $(if ($Abort) { '' } else { "`n" }))
        $stream.Write($request, 0, $request.Length)
        if ($Abort) {
            # RST after an incomplete line, not a normally completed exchange.
            $tcp.Client.LingerState = [Net.Sockets.LingerOption]::new($true, 0)
            $code = 'ABORT_SENT'
        } else {
            [byte[]]$expected = [Text.Encoding]::ASCII.GetBytes("ECHO $Payload`n")
            [byte[]]$reply = [byte[]]::new($expected.Length)
            $n = 0
            while ($n -lt $reply.Length) {
                $remaining = $TimeoutMs - [int]$timer.ElapsedMilliseconds
                if ($remaining -le 0) { throw 'TIMEOUT' }
                $stream.ReadTimeout = $remaining
                $got = $stream.Read($reply, $n, $reply.Length - $n)
                if ($got -eq 0) { throw 'LENGTH' }
                $n += $got
            }
            for ($i = 0; $i -lt $n; $i++) { if ($reply[$i] -ne $expected[$i]) { throw 'CORRUPT' } }
            $remaining = $TimeoutMs - [int]$timer.ElapsedMilliseconds
            if ($remaining -le 0) { throw 'TIMEOUT' }
            $stream.ReadTimeout = $remaining
            if ($stream.ReadByte() -ne -1) { throw 'EXTRA_BYTES' }
        }
    } catch {
        $detail = $_.Exception.Message
        $code = if ($detail -match 'CORRUPT|LENGTH|EXTRA_BYTES') { 'CORRUPT' } else { 'TIMEOUT_OR_SOCKET' }
    } finally { $tcp.Dispose() }
    [pscustomobject]@{ Code = $code; Detail = $detail; Ms = $timer.Elapsed.TotalMilliseconds }
}

function Get-NetworkVerdict {
    param([System.Collections.IDictionary]$Metrics)
    $reasons = [Collections.Generic.List[string]]::new()
    if (-not $Metrics.Completed) { $reasons.Add('bounded run did not finish') }
    if ($Metrics.Fatal) { $reasons.Add("runner error: $($Metrics.Fatal)") }
    if ($Metrics.Stats -lt 2) { $reasons.Add('insufficient UART statistics') }
    if ($Metrics.Boots -ne 1) { $reasons.Add('expected exactly one observed BOOT') }
    if (-not $Metrics.Firmware) { $reasons.Add('TEST-07 v0.1 firmware banner not observed') }
    if (-not $Metrics.W5100) { $reasons.Add('W5100 not observed') }
    if ($Metrics.MinFree -lt 512) { $reasons.Add('sampled free SRAM below 512 B') }
    if ($Metrics.RamDrift -gt 64) { $reasons.Add('sampled free SRAM fell by more than 64 B') }
    if ($Metrics.Corrupt -gt 0) { $reasons.Add('wrong source, length or data') }
    if ($Metrics.HealthyFail -gt 0) { $reasons.Add('exchange failure outside warmup/outage/recovery') }
    if ($Metrics.HealthyOK -lt 24 -or $Metrics.TcpOK -lt 3 -or $Metrics.Boundaries -lt 4) {
        $reasons.Add('insufficient successful exchanges or boundary coverage')
    }
    if ($Metrics.SendFail -gt 0 -or $Metrics.Drop -gt 0) { $reasons.Add('firmware send/drop error') }
    if ($Metrics.ManualDhcp -gt 0) { $reasons.Add('UART-triggered DHCP cannot certify automatic recovery') }
    if ($Metrics.Scenario -in @('Cable','StartupDhcp','DhcpOutage')) {
        if (-not $Metrics.Restored -or -not $Metrics.Recovered -or $Metrics.RecoveryMs -gt $Metrics.RecoveryLimitMs) {
            $reasons.Add('recovery absent or over time limit')
        }
        if ($Metrics.PostOK -lt 24 -or $Metrics.PostTcpOK -lt 3) { $reasons.Add('insufficient post-recovery integrity checks') }
        if ($Metrics.OutageMs -lt 10000) { $reasons.Add('outage shorter than 10 seconds') }
    }
    if ($Metrics.Scenario -eq 'Cable' -and $Metrics.OutageFailures -lt 3) { $reasons.Add('cable outage not demonstrated') }
    if ($Metrics.Scenario -eq 'StartupDhcp' -and $Metrics.DhcpFailuresBeforeRestore -lt 1) {
        $reasons.Add('startup DHCP failure not observed before restoration')
    }
    if ($Metrics.Scenario -eq 'DhcpOutage' -and $Metrics.MaintainFailuresBeforeRestore -lt 1) {
        $reasons.Add('natural DHCP maintenance failure not observed before restoration')
    }
    if ($Metrics.Scenario -eq 'DhcpRenew' -and $Metrics.RenewOK -lt 1) {
        $reasons.Add('renew rc=2 absent (rc=0, rc=4 or a fresh begin is not renewal evidence)')
    }
    if ($Metrics.Scenario -eq 'TcpAbort' -and ($Metrics.Aborts -lt 5 -or $Metrics.TcpReject -lt 5)) {
        $reasons.Add('five incomplete TCP aborts and firmware rejections required')
    }
    if ($Metrics.Scenario -eq 'Soak' -and ($Metrics.DurationSeconds -lt 600 -or $Metrics.HealthySeconds -lt 540)) {
        $reasons.Add('soak requires >=600 s run and >=540 s healthy service')
    }
    [pscustomobject]@{ Status = $(if ($reasons.Count) { 'FAIL' } else { 'PASS' }); Reasons = @($reasons) }
}
Export-ModuleMember -Function ConvertFrom-NetworkLine,New-ProbePayload,Test-UdpProbe,Test-TcpProbe,Get-NetworkVerdict
