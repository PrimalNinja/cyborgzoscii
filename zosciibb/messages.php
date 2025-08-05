<?php
define("MSG_DIR", "messages/");

function listMessages($intLimit_a, $intPage_a)
{
    $arrFiles = array();
    if (is_dir(MSG_DIR))
    {
        $arrAll = scandir(MSG_DIR);
        foreach ($arrAll as $strFilename)
        {
            if (preg_match('/\\.bin$/i', $strFilename))
            {
                $arrFiles[] = $strFilename;
            }
        }
        rsort($arrFiles, SORT_NATURAL | SORT_FLAG_CASE);
    }
    $intOffset = ($intPage_a - 1) * $intLimit_a;
    $arrFiles = array_slice($arrFiles, $intOffset, $intLimit_a);
    $arrResults = array();
    foreach ($arrFiles as $strFilename)
    {
        // Extract date from filename YYYYMMDDHHNNSSCC
        $strDate = substr($strFilename, 0, 14); // Get YYYYMMDDHHNNSS part
        $intTimestamp = 0;
        if (strlen($strDate) == 14)
        {
            $intTimestamp = mktime(
                intval(substr($strDate, 8, 2)),  // hour
                intval(substr($strDate, 10, 2)), // minute
                intval(substr($strDate, 12, 2)), // second
                intval(substr($strDate, 4, 2)),  // month
                intval(substr($strDate, 6, 2)),  // day
                intval(substr($strDate, 0, 4))   // year
            );
        }
        
        $arrResults[] = array(
            "binFilename" => $strFilename,
            "date" => $intTimestamp
        );
    }
    header("Content-Type: application/json");
    echo json_encode($arrResults);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] === "GET")
{
    if (isset($_GET['limit']))
    {
        $intLimit = intval($_GET['limit']);
        if ($intLimit < 1)
        {
            $intLimit = 1;
        }
        else if ($intLimit > 100)
        {
            $intLimit = 100;
        }
    }
    else
    {
        $intLimit = 20;
    }
    
    if (isset($_GET['page']))
    {
        $intPage = intval($_GET['page']);
        if ($intPage < 1)
        {
            $intPage = 1;
        }
    }
    else
    {
        $intPage = 1;
    }
    
    listMessages($intLimit, $intPage);
}

if ($_SERVER['REQUEST_METHOD'] === "POST" && isset($_FILES['addressfile']))
{
    $strDate = date("YmdHis");
    $intCounter = 0;
    do
    {
        $strBinFile = $strDate . sprintf("%02d", $intCounter) . ".bin";
        $strPath = MSG_DIR . $strBinFile;
        $intCounter++;
    }
    while(file_exists($strPath) && $intCounter < 100);
    
    move_uploaded_file($_FILES['addressfile']['tmp_name'], $strPath);
    
    header("Content-Type: application/json");
    echo json_encode(array("success" => true, "binFile" => $strBinFile));
    exit;
}

header('HTTP/1.1 400 Bad Request');
echo json_encode(array("error" => "Bad request"));
exit;
?>