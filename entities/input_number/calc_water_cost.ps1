$ColdWaterUsageM3 = 55.140
$HotWaterUsageM3  = 20.788

$ColdWaterTotalCost = 11175.21 # (CZK)
$HotWaterTotalCost = 99.20 # (CZK)

$ColdWaterM3Cost = $ColdWaterTotalCost / ($ColdWaterUsageM3 + $HotWaterUsageM3)
$HotWaterM3Cost = $ColdWaterM3Cost + ($HotWaterTotalCost / $HotWaterUsageM3)

New-Object PSCustomObject -Property @{
    ColdWaterCost = $ColdWaterM3Cost
    HotWaterCost = $HotWaterM3Cost
}

