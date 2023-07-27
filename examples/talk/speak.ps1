# Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope CurrentUser
param(
  # voice options are David or Zira
  [Parameter(Mandatory=$true)][string]$voice,
  [Parameter(Mandatory=$true)][string]$text
)

Add-Type -AssemblyName System.Speech;
$speak = New-Object System.Speech.Synthesis.SpeechSynthesizer;
$speak.SelectVoice("Microsoft $voice Desktop");
$speak.Rate="0";
$speak.Speak($text);
