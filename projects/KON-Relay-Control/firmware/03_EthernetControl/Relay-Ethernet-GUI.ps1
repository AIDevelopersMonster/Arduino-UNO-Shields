param([string]$IP='')

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[System.Windows.Forms.Application]::EnableVisualStyles()

$form=New-Object System.Windows.Forms.Form
$form.Text='KON W5100 4-Relay Controller'
$form.Size=New-Object System.Drawing.Size(560,430)
$form.StartPosition='CenterScreen'
$form.FormBorderStyle='FixedDialog'
$form.MaximizeBox=$false

$l=New-Object System.Windows.Forms.Label
$l.Text='Controller IP:'
$l.Location=New-Object System.Drawing.Point(18,20)
$l.Size=New-Object System.Drawing.Size(90,24)
$form.Controls.Add($l)

$txt=New-Object System.Windows.Forms.TextBox
$txt.Text=$IP
$txt.Location=New-Object System.Drawing.Point(110,18)
$txt.Size=New-Object System.Drawing.Size(190,24)
$form.Controls.Add($txt)

$refresh=New-Object System.Windows.Forms.Button
$refresh.Text='Connect / Refresh'
$refresh.Location=New-Object System.Drawing.Point(315,16)
$refresh.Size=New-Object System.Drawing.Size(140,28)
$form.Controls.Add($refresh)

$status=New-Object System.Windows.Forms.Label
$status.Text='Not connected'
$status.Location=New-Object System.Drawing.Point(18,55)
$status.Size=New-Object System.Drawing.Size(500,24)
$form.Controls.Add($status)

$stateLabels=@()

function Base-Url {
  $v=$txt.Text.Trim()
  if([string]::IsNullOrWhiteSpace($v)){ throw 'Enter controller IP address.' }
  "http://$v"
}

function Refresh-State {
  try {
    $s=Invoke-RestMethod -Uri "$(Base-Url)/api/status" -TimeoutSec 3
    for($i=0;$i -lt 4;$i++){
      $stateLabels[$i].Text=$(if($s.relay[$i]){'ON'}else{'OFF'})
    }
    $logic=$(if($s.active_low){'ACTIVE LOW'}else{'ACTIVE HIGH'})
    $status.Text="Connected: $($s.ip)   Logic: $logic   Uptime: $($s.uptime_s)s"
  } catch {
    $status.Text="ERROR: $($_.Exception.Message)"
  }
}

function Send-Relay([int]$Channel,[string]$Action){
  try {
    Invoke-RestMethod -Uri "$(Base-Url)/api/r$Channel/$Action" -TimeoutSec 3 | Out-Null
    Refresh-State
  } catch {
    $status.Text="ERROR: $($_.Exception.Message)"
  }
}

function Send-All([string]$Action){
  try {
    Invoke-RestMethod -Uri "$(Base-Url)/api/all/$Action" -TimeoutSec 3 | Out-Null
    Refresh-State
  } catch {
    $status.Text="ERROR: $($_.Exception.Message)"
  }
}

$relayClick={
  param($sender,$eventArgs)
  $p=$sender.Tag.ToString().Split('|')
  Send-Relay ([int]$p[0]) $p[1]
}

for($i=0;$i -lt 4;$i++){
  $y=90+($i*58)

  $name=New-Object System.Windows.Forms.Label
  $name.Text="Relay $($i+1)"
  $name.Location=New-Object System.Drawing.Point(20,$($y+7))
  $name.Size=New-Object System.Drawing.Size(65,24)
  $form.Controls.Add($name)

  $st=New-Object System.Windows.Forms.Label
  $st.Text='?'
  $st.Font=New-Object System.Drawing.Font('Segoe UI',10,[System.Drawing.FontStyle]::Bold)
  $st.Location=New-Object System.Drawing.Point(90,$($y+7))
  $st.Size=New-Object System.Drawing.Size(45,24)
  $form.Controls.Add($st)
  $stateLabels += $st

  foreach($spec in @(
    @{T='ON';A='on';X=145},
    @{T='OFF';A='off';X=235},
    @{T='TOGGLE';A='toggle';X=325}
  )){
    $b=New-Object System.Windows.Forms.Button
    $b.Text=$spec.T
    $b.Location=New-Object System.Drawing.Point($spec.X,$y)
    $b.Size=New-Object System.Drawing.Size(80,34)
    $b.Tag="$($i+1)|$($spec.A)"
    $b.Add_Click($relayClick)
    $form.Controls.Add($b)
  }
}

$allOn=New-Object System.Windows.Forms.Button
$allOn.Text='ALL ON'
$allOn.Location=New-Object System.Drawing.Point(145,330)
$allOn.Size=New-Object System.Drawing.Size(120,36)
$allOn.Add_Click({Send-All 'on'})
$form.Controls.Add($allOn)

$allOff=New-Object System.Windows.Forms.Button
$allOff.Text='ALL OFF'
$allOff.Location=New-Object System.Drawing.Point(285,330)
$allOff.Size=New-Object System.Drawing.Size(120,36)
$allOff.Add_Click({Send-All 'off'})
$form.Controls.Add($allOff)

$refresh.Add_Click({Refresh-State})
$form.Add_Shown({if(-not [string]::IsNullOrWhiteSpace($txt.Text)){Refresh-State}})

[void]$form.ShowDialog()
