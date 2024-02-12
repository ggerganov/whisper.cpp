# Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope CurrentUser
param(
  [Parameter(Mandatory=$true)][string]$textfile
)

Add-Type -AssemblyName System.Speech;
$speak = New-Object System.Speech.Synthesis.SpeechSynthesizer;
# voice options are David or Zira
$voice = "Zira"
$speak.SelectVoice("Microsoft $voice Desktop");
$speak.Rate="0";
$text = Get-Content -Path $textfile
$speak.Speak($text);
