<?php
	$strWikiPage = $_GET['wiki'] ?? '';
	
	if (strlen($strWikiPage) == 0)
	{
		$strWikiPage = "index";
	}

	if ($strWikiPage) 
	{
		$strWikiPage = filter_var($strWikiPage, FILTER_SANITIZE_STRING);
		$strWikiFile = $strWikiPage . '.wiki';

		if (file_exists('pages/' . $strWikiFile)) 
		{
			$strWikiContent = file_get_contents('pages/' . $strWikiFile);
			$strWikiContent = str_replace("\t", "    ", $strWikiContent); 
		} 
		else 
		{
			echo 'Wiki page not found.';
		}
	} 
	else 
	{
		echo 'No wiki page specified.';
	}
?>
<!DOCTYPE html>
<html>
	<head>
		<title>ZOSCII - ZERO encryption ZERO ciphering 100% SECURITY</title>
		<link rel="stylesheet" type="text/css" href="css/cyborgWiki.css">
	</head>
	<body>
		<div class="gscw-container">
			<div class="gscw-container-rounder">
				<div class="gscw-container-scroller">
					<div class="gscw-content" id="output"></div>
				</div>
			</div>
		</div>

		<script src="js/cyborgWiki.min.js"></script>
		<script>
			var strWikiContent = <?php echo json_encode($strWikiContent); ?>;

			var arrPlaceholders = [
				{ placeholder: "%APPNAME%", value: "Cyborg Wiki" }
			];

			var strHtml = cyborgWikiToHtml(strWikiContent, "?wiki=", "images/", arrPlaceholders);
			document.getElementById("output").innerHTML = strHtml;
		</script>
	</body>
</html>