<?php 

define('DEBUG', 'FALSE'); 		// TRUE or FALSE
define('LOG_OUTPUT', 'FALSE');	// TRUE or FALSE

define('LOCAL_URL', 'http://localhost/zosciimq/index.php');	// local republishing goes here
define('FOLDER_PERMISSIONS', 755);

define('NONCE_ROOT', './nonce/');
define('QUEUE_ROOT', './queues/'); 
define('STORE_ROOT', './store/'); 

define('TEMP_QUEUE', 'temp/');

define('NONCE_TIMEFRAME', 5);	// minimum minutes to keep NONCE

