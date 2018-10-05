param(
    [Parameter(Position=0, Mandatory=$true)]
    $Target,

    [Parameter(Position=1, Mandatory=$true)]
    $SolutionDir
)

function Get-ContentBytesAndXor {
    [CmdletBinding()]
    Param(
        [Parameter(Position=0, Mandatory=$true)]
        [string]
        $Path,

        [Parameter(Position=1, Mandatory=$false)]
        [int]
        $XorKey = 0
    )

    Write-Verbose "XorKey: $XorKey" 
    if(Test-Path -Path $Path -ErrorAction Stop -PathType Leaf) {
        $Fullname = (Get-ChildItem $Path).FullName

        if($XorKey -ne 0) {
            Write-Verbose "Using XorKey"
            foreach($Byte in ([System.IO.File]::ReadAllBytes($Fullname))) {
                $Byte -bxor $XorKey
            }
        } else {
            [System.IO.File]::ReadAllBytes($FullName)
        }
    }
}

function ConvertTo-CSharpDataClass {
    [CmdletBinding()]
    Param(
        [Parameter(Position=0, Mandatory=$true)]
        [byte[]]
        $ByteArray,

        [Parameter(Position=1, Mandatory=$false)]
        [int]
        $Columns = 20,

        [Parameter(Position=2, Mandatory=$false)]
        [string]
        $Namespace = 'Test',

        [Parameter(Position=3, Mandatory=$false)]
        [string]
        $Class = 'Data',

        [Parameter(Position=4, Mandatory=$false)]
        [string]
        $Variable = 'Data'
    )


    $sb = New-Object System.Text.StringBuilder

    $null = $sb.Append(@"
namespace SpoolSample
{
    static class Data
    {
        public static byte[] $($Variable) = new byte[] {
"@)

    for($i = 0; $i -lt $ByteArray.Count; $i++) {
        if(($i % $Columns) -eq 0) {
            $null = $sb.Append("`n            ")
        }
        $null = $sb.Append("0x{0:X2}," -f $ByteArray[$i])
    }

    $null = $sb.Remove($sb.Length-1, 1) #Remove the last comman
    $null = $sb.AppendLine(@"
`n        };
    }
}
"@)

    $sb.ToString()
}


# For whatever reason, VS keeps including a double quote in the pathst
$SolutionDir = $SolutionDir.Replace('"','')

$bytes = Get-ContentBytesAndXor -Path $Target
ConvertTo-CSharpDataClass -Namespace 'SpoolSample' -Class 'Data' -Variable 'RprnDll' -ByteArray $bytes | Out-File -Encoding ascii -FilePath "$($SolutionDir)\SpoolSample\Data.cs"