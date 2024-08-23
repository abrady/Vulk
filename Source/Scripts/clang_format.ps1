# run this in the Source directory
# Set the directory to start searching from
# Set the required path
$requiredPath = "Vulk\Source"

# Get the current directory
$rootDir = Get-Location

# Check if the current directory is under the required path
echo ($rootDir.Path)
if ($rootDir.Path -notlike "*\$requiredPath") {
    Write-Host "Error: You must run this script from a directory under $requiredPath"
    exit
}


# Set the file extensions to format
$extensions = @(".cpp", ".h")

# Get all files with the specified extensions
$files = Get-ChildItem -Path $rootDir -Filter * -Recurse -File | Where-Object { $_.Extension -in $extensions }

# Run clang-format on each file
foreach ($file in $files) {
    $command = "clang-format -i -style=file $file"
    Write-Verbose "Running: $command"
    Invoke-Expression $command
}