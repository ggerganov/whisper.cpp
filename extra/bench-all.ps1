# Helper script to run the bench tool on all models and print the results in share-able format

Write-Host "Usage: .\bench-all.ps1 [n_threads]`n"

if ($args.Length -eq 0) {
    $n_threads = 4
} else {
    $n_threads = $args[0]
}

$models = @( "tiny", "base", "small", "medium", "large" )

Write-Host "`nRunning memcpy benchmark with 1 thread`n"

.\bench -w 1 -t 1 2>&1

Write-Host "`nRunning ggml_mul_mat benchmark with $n_threads threads`n"

.\bench -w 2 -t $n_threads 2>&1

Write-Host "`nRunning benchmark for all models"
Write-Host "This can take a while!`n"

Write-Host "| CPU | OS | Config | Model | Th | Load | Enc. | Commit |"
Write-Host "| --- | -- | ------ | ----- | -- | ---- | ---- | ------ |"

$commit = git rev-parse --short HEAD
$cpu_name = (Get-CimInstance -ClassName Win32_Processor).Name
$os = ((Get-WMIObject win32_operatingsystem).name).split("|")[0]

foreach ($model in $models) {
    # run once to heat-up the cache
    .\bench -m .\models\ggml-$model.bin -t $n_threads 2>&1 | Out-Null

    # actual run
    # store stderr output in a variable in order to parse it later
    $output = .\bench -m .\models\ggml-$model.bin -t $n_threads 2>&1

    # parse the output:
    $load_time = (($output | Select-String "load time") -split "[\s]+")[4]
    $encode_time =  (($output | Select-String "encode time") -split "[\s]+")[4]
    $system_info = ($output | Select-String "system_info").ToString()
    $n_threads = ($system_info -split "[\s]+")[3]

    # convert the time values to float type
    $load_time = [float]::Parse($load_time)
    $encode_time = [float]::Parse($encode_time)

    # floor to milliseconds
    $load_time = [math]::Floor($load_time)
    $encode_time = [math]::Floor($encode_time)

    $config = ""

    if ($system_info.Contains("AVX2 = 1")) {
        $config += " AVX2"
    }

    if ($system_info.Contains("NEON = 1")) {
        $config += " NEON"
    }

    if ($system_info.Contains("BLAS = 1")) {
        $config += " BLAS"
    }

    Write-Host "| $cpu_name | $os |$config | $model | $n_threads | $load_time | $encode_time | $commit |"
}