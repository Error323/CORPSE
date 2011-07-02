<?php
	define("ROOT", $_SERVER['DOCUMENT_ROOT']."/corpse/");

	include("header.html");

    if (isset($_GET['page']))
        $file = ROOT."pages/".$_GET['page'].".html";
    else
        $file = null;

	if (file_exists($file))
		include($file);
	else
		include(ROOT."pages/introduction.html");

	include("footer.html");

	unset($file);
?>
