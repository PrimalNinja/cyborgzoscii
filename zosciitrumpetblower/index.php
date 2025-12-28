<?php
// server-side: messages.php for TrumpetBlower

// Add CORS headers to allow local file access
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

// Handle preflight requests
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') 
{
    http_response_code(200);
    exit;
}

define("MSG_BASE_DIR", "data/");

function listMessages($strChannel_a, $intLimit_a, $intPage_a)
{
    $arrFiles = array();
    $strChannelDir = MSG_BASE_DIR . $strChannel_a . '/';
    if (is_dir($strChannelDir))
	{
        $arrAll = scandir($strChannelDir);
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

function serveMessage($strChannel_a, $strFilename_a) 
{
    // Validate channel (UUID format: 8-4-4-4-12 characters)
    //if (!preg_match('/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i', $strChannel_a))
	if (!preg_match('/^[0-9]+$/', $strChannel_a))
	{
        http_response_code(400);
        header("Content-Type: application/json");
        echo json_encode(array("error" => "Invalid channel number"));
        exit;
    }
	
    // Security: only allow .bin files and prevent directory traversal
    //if (!preg_match('/^[0-9]{16}\.bin$/', $strFilename_a))
	if (!preg_match('/^[0-9]{16}-[0-9]+\.bin$/', $strFilename_a))
	{
        http_response_code(400);
        header("Content-Type: application/json");
        echo json_encode(array("error" => "Invalid filename"));
        exit;
    }
	
    $strFilepath = MSG_BASE_DIR . $strChannel_a . '/' . $strFilename_a;
    if (file_exists($strFilepath))
	{
        header('Content-Type: application/octet-stream');
        header('Content-Length: ' . filesize($strFilepath));
        readfile($strFilepath);
        exit;
    } 
	else 
	{
        http_response_code(404);
        header("Content-Type: application/json");
        echo json_encode(array("error" => "File not found"));
        exit;
    }
}

if ($_SERVER['REQUEST_METHOD'] === "GET")
{
    // Serve landing page
    if (!isset($_GET['file']) && !isset($_GET['limit']))
	{
        header("Content-Type: text/html");
        echo "<h1>TrumpetBlower Server</h1><p>Read the <a href='readme.html'>instructions</a>.</p><p>Download the client from <a href='https://github.com/PrimalNinja/cyborgzoscii'>GitHub</a> to blow your trumpet!</p><p>Browse and download ZOSCII Trumpet Address files from <a href='data'>here</a>.</p>";
        exit;
    }
    
	// Handle message file serving
    if (isset($_GET['file']) && isset($_GET['channel']))
	{
        serveMessage($_GET['channel'], $_GET['file']);
    }
    
	// Handle message list
    if (isset($_GET['limit']) && isset($_GET['channel']))
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
		
        $intPage = isset($_GET['page']) ? intval($_GET['page']) : 1;
        if ($intPage < 1)
		{
            $intPage = 1;
        }
        listMessages($_GET['channel'], $intLimit, $intPage);
    }
	
    http_response_code(400);
    header("Content-Type: application/json");
    echo json_encode(array("error" => "Missing channel or parameters"));
    exit;
}

if ($_SERVER['REQUEST_METHOD'] === "POST" && isset($_FILES['addressfile']) && isset($_POST['channel']) && isset($_POST['retention']))
{
    $strChannel = $_POST['channel'];
    if (!preg_match('/^[0-9]+$/', $strChannel))
	{
		error_log("Invalid channel number: $strChannel");
        http_response_code(400);
        header("Content-Type: application/json");
        echo json_encode(array("error" => "Invalid channel number"));
        exit;
    }

    $strRetention = $_POST['retention'];
    if (!preg_match('/^[0-9]+$/', $strRetention))
	{
		error_log("Invalid retention number: $strRetention");
        http_response_code(400);
        header("Content-Type: application/json");
        echo json_encode(array("error" => "Invalid retention number"));
        exit;
    }

    $strChannelDir = MSG_BASE_DIR . $strChannel . '/';
    if (!is_dir($strChannelDir))
	{
        if (!mkdir($strChannelDir, 0755, true))
		{
            http_response_code(500);
            header("Content-Type: application/json");
            echo json_encode(array("error" => "Failed to create channel directory"));
            exit;
        }
		
		error_log("Directory created: $strChannelDir");
    }

    $strDate = date("YmdHis");
    $intCounter = 0;
    do
	{
        $strBinFile = $strDate . sprintf("%02d", $intCounter) . "-" . $strRetention . ".bin";
        $strPath = $strChannelDir . $strBinFile;
        $intCounter++;
    }
	while (file_exists($strPath) && $intCounter < 100);

    if (!move_uploaded_file($_FILES['addressfile']['tmp_name'], $strPath))
	{
        http_response_code(500);
        header("Content-Type: application/json");
        echo json_encode(array("error" => "Failed to upload file"));
        exit;
    }
	error_log("File moved successfully");

    header("Content-Type: application/json");
    echo json_encode(array("success" => true, "binFile" => $strBinFile));
    exit;
}

header('HTTP/1.1 400 Bad Request');
header("Content-Type: application/json");
echo json_encode(array("error" => "Bad request"));
exit;
?>