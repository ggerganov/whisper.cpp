# Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope CurrentUser
param(
  [Parameter(Mandatory=$true)][int]$voicenum,
  [Parameter(Mandatory=$true)][string]$textfile
)

Add-Type -AssemblyName System.Speech;
$speak = New-Object System.Speech.Synthesis.SpeechSynthesizer;
$voiceoptions = "David", "Zira";
$voice = $voiceoptions[$voicenum % $voiceoptions.count];
$speak.SelectVoice("Microsoft $voice Desktop");
$speak.Rate="0";
$text = Get-Content -Path $textfile;
$speak.Speak($text);
