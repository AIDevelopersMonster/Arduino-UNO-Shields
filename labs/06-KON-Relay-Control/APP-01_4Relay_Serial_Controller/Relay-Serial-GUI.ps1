param([string]$Port='COM4',[int]$Baud=115200)

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[System.Windows.Forms.Application]::EnableVisualStyles()

$sp = New-Object System.IO.Ports.SerialPort
$sp.BaudRate=$Baud
$sp.NewLine=[Environment]::NewLine
$sp.ReadTimeout=300
$sp.WriteTimeout=1000
$sp.DtrEnable=$false
$sp.RtsEnable=$false

$form=New-Object System.Windows.Forms.Form
$form.Text='KON 4-Relay Serial Controller'
$form.Size=New-Object System.Drawing.Size(600,440)
$form.StartPosition='CenterScreen'
$form.FormBorderStyle='FixedDialog'
$form.MaximizeBox=$false

$txtPort=New-Object System.Windows.Forms.TextBox
$txtPort.Text=$Port
$txtPort.Location=New-Object System.Drawing.Point(20,18)
$txtPort.Size=New-Object System.Drawing.Size(90,24)
$form.Controls.Add($txtPort)

$btnConnect=New-Object System.Windows.Forms.Button
$btnConnect.Text='Connect'
$btnConnect.Location=New-Object System.Drawing.Point(125,16)
$btnConnect.Size=New-Object System.Drawing.Size(95,28)
$form.Controls.Add($btnConnect)

$btnStatus=New-Object System.Windows.Forms.Button
$btnStatus.Text='STATUS'
$btnStatus.Location=New-Object System.Drawing.Point(235,16)
$btnStatus.Size=New-Object System.Drawing.Size(90,28)
$form.Controls.Add($btnStatus)

$lblConn=New-Object System.Windows.Forms.Label
$lblConn.Text='Disconnected'
$lblConn.Location=New-Object System.Drawing.Point(345,20)
$lblConn.Size=New-Object System.Drawing.Size(220,24)
$form.Controls.Add($lblConn)

$states=@()
$log=New-Object System.Windows.Forms.TextBox
$log.Location=New-Object System.Drawing.Point(20,300)
$log.Size=New-Object System.Drawing.Size(550,90)
$log.Multiline=$true
$log.ScrollBars='Vertical'
$log.ReadOnly=$true
$form.Controls.Add($log)

function Append-Log([string]$t){ if($t){ $log.AppendText($t + [Environment]::NewLine) } }
function Update-State([string]$t){
  if($t -match 'STATE\s+R1=(ON|OFF)\s+R2=(ON|OFF)\s+R3=(ON|OFF)\s+R4=(ON|OFF)'){
    for($i=0;$i -lt 4;$i++){ $states[$i].Text=$Matches[$i+1] }
  }
}
function Read-Reply { Start-Sleep -Milliseconds 120; $r=$sp.ReadExisting(); if($r){ Update-State $r; Append-Log $r } }
function Send-Cmd([string]$c){
  if(-not $sp.IsOpen){ [System.Windows.Forms.MessageBox]::Show('Connect to COM port first.') | Out-Null; return }
  try { $sp.WriteLine($c); Append-Log ('> '+$c); Read-Reply } catch { Append-Log $_.Exception.Message }
}

$relayClick={ param($sender,$eventArgs); $p=$sender.Tag.ToString().Split('|'); Send-Cmd ('R'+$p[0]+' '+$p[1]) }

for($i=0;$i -lt 4;$i++){
  $y=65+($i*52)
  $name=New-Object System.Windows.Forms.Label
  $name.Text='Relay '+($i+1)
  $name.Location=New-Object System.Drawing.Point(20,$($y+7))
  $name.Size=New-Object System.Drawing.Size(70,24)
  $form.Controls.Add($name)
  $st=New-Object System.Windows.Forms.Label
  $st.Text='?'
  $st.Location=New-Object System.Drawing.Point(95,$($y+7))
  $st.Size=New-Object System.Drawing.Size(45,24)
  $form.Controls.Add($st)
  $states += $st
  foreach($spec in @(@{T='ON';A='ON';X=150},@{T='OFF';A='OFF';X=245},@{T='TOGGLE';A='TOGGLE';X=340})){
    $b=New-Object System.Windows.Forms.Button
    $b.Text=$spec.T
    $b.Location=New-Object System.Drawing.Point($spec.X,$y)
    $b.Size=New-Object System.Drawing.Size(85,32)
    $b.Tag=(($i+1).ToString()+'|'+$spec.A)
    $b.Add_Click($relayClick)
    $form.Controls.Add($b)
  }
}

$allOn=New-Object System.Windows.Forms.Button
$allOn.Text='ALL ON'
$allOn.Location=New-Object System.Drawing.Point(150,270)
$allOn.Size=New-Object System.Drawing.Size(120,32)
$allOn.Add_Click({Send-Cmd 'ALL ON'})
$form.Controls.Add($allOn)
$allOff=New-Object System.Windows.Forms.Button
$allOff.Text='ALL OFF'
$allOff.Location=New-Object System.Drawing.Point(285,270)
$allOff.Size=New-Object System.Drawing.Size(120,32)
$allOff.Add_Click({Send-Cmd 'ALL OFF'})
$form.Controls.Add($allOff)

$btnConnect.Add_Click({
  try {
    if($sp.IsOpen){ $sp.Close(); $btnConnect.Text='Connect'; $lblConn.Text='Disconnected'; return }
    $sp.PortName=$txtPort.Text.Trim()
    $sp.Open()
    Start-Sleep -Milliseconds 250
    $btnConnect.Text='Disconnect'
    $lblConn.Text=$sp.PortName+' @ '+$Baud
    Read-Reply
    Send-Cmd 'STATUS'
  } catch { $lblConn.Text='ERROR'; Append-Log $_.Exception.Message }
})
$btnStatus.Add_Click({Send-Cmd 'STATUS'})
$form.Add_FormClosing({ if($sp.IsOpen){$sp.Close()}; $sp.Dispose() })
[void]$form.ShowDialog()
