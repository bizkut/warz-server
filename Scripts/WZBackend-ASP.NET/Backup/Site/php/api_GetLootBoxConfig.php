<?php
	$serverkey = !isset($_POST['serverkey']) ? "" : $_POST['serverkey'];
	$serverkey2 = !isset($_GET['serverkey']) ? "" : $_GET['serverkey'];
	$key = "Fg5jaBgj3uy3ja";
	if ($serverkey != $key && $serverkey2 != $key)
	{
		die('oops');
	}

	require_once('dbinfo.inc.php');

	header("Content-type: text/xml");
	$xml_output = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

	// create & execute query: LOOTBOXES
	$tsql   = "select * from Items_Generic where Category=7";
	$params = array();
	$stmt   = sqlsrv_query($conn, $tsql, $params);

	$xml_output .= "<LootBoxDB>\n";
	while($member = sqlsrv_fetch_array($stmt, SQLSRV_FETCH_ASSOC))
	{
		$itemid=$member['ItemID'];

		// get server loot chances
		$SrvNormal  = 100;
		$SrvTrial   = 100;
		$SrvPremium = 200;
		$tsql2   = "select * from Items_LootSrvModifiers where LootID=$itemid";
		$params2 = array();
		$stmt2   = sqlsrv_query($conn, $tsql2, $params2);
		while($member2 = sqlsrv_fetch_array($stmt2, SQLSRV_FETCH_ASSOC))
		{
			$SrvNormal  = $member2['SrvNormal'];
			$SrvTrial   = $member2['SrvTrial'];
			$SrvPremium = $member2['SrvPremium'];
		}

		$xml_output .= "\t<LootBox itemID=\"$itemid\" SrvNormal=\"$SrvNormal\" SrvTrial=\"$SrvTrial\" SrvPremium=\"$SrvPremium\"> \n";

		$tsql2   = "select * from Items_LootData where LootID=$itemid order by Chance asc";
		$params2 = array();
		$stmt2   = sqlsrv_query($conn, $tsql2, $params2);

		while($member2 = sqlsrv_fetch_array($stmt2, SQLSRV_FETCH_ASSOC))
		{
			$l_Chance = $member2['Chance'];
			$l_ItemID = $member2['ItemID'];
			$l_GDMin  = $member2['GDMin'];
			$l_GDMax  = $member2['GDMax'];

			$xml_output .= "\t\t<d c=\"$l_Chance\" i=\"$l_ItemID\" g1=\"$l_GDMin\" g2=\"$l_GDMax\" />\n";
		}
	  	$xml_output .= "\t</LootBox>\n";
	}
	$xml_output .= "</LootBoxDB>\n\n";

	echo $xml_output;
	exit();
?>