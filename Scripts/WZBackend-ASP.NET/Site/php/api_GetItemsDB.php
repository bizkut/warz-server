<?php
	$serverkey = $_POST['serverkey'];
	$ServerMode = 0;
	if ($serverkey == "F1F5F7BA-0E83-47c1-B670-425B62D8F46E")
	{
		$ServerMode = 1;
	}
	if ($serverkey == "Fg5jaBgj3uy3ja")
	{
		$ServerMode = 2;
	}
	if($ServerMode == 0)
	{
		die('oops');
	}

	header("Content-type: text/xml");

	$xml_output = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	$xml_output .= "<DB>\n";

	require_once('dbinfo.inc.php');

	// create & execute query: WEAPONS
	$tsql   = "select * from Items_Weapons where Category!=30 AND Category!=32 AND Category!=33 AND Category!=55"; // except food\water\medical
	$params = array();
	$stmt   = sqlsrv_query($conn, $tsql, $params);

	function xml_addvar($name, $db_name)
	{
		global $member;
		global $xml_output;

		$xml_output .= " $name=\"" . $member[$db_name] . "\"";
	}

	function out_durability()
	{
		global $member;
		global $xml_output;

		if($member['DurabilityUse'] == 0 and $member['RepairAmount'] == 0)
			return;

		$xml_output .= "\t<Dur";
		xml_addvar('u', 'DurabilityUse');
		xml_addvar('r1', 'RepairAmount');
		xml_addvar('r2', 'PremRepairAmount');
		xml_addvar('r3', 'RepairPriceGD');
		$xml_output .= "/>\n";
	}

	function out_disassembly()
	{
		global $member;
		global $xml_output;

		$r1 = $member['ResWood'];
		$r2 = $member['ResStone'];
		$r3 = $member['ResMetal'];
		if($r1 == 0 && $r2 == 0 && $r3 == 0)
			return;

		$xml_output .= "\t<Res";
		xml_addvar('r1', 'ResWood');
		xml_addvar('r2', 'ResStone');
		xml_addvar('r3', 'ResMetal');
		$xml_output .= "/>\n";
	}

	$xml_output .= "<WeaponsArmory>\n";
	while($member = sqlsrv_fetch_array($stmt, SQLSRV_FETCH_ASSOC))
	{
		$itemid=$member['ItemID'];
		$fname=str_replace (" ", "",$member['FNAME']);
		$cat=$member['Category'];
		$name=htmlentities($member['Name'], ENT_COMPAT, 'UTF-8');
		$desc=htmlentities($member['Description'], ENT_COMPAT, 'UTF-8');
		$desc=str_replace("\r\n", "&#xD;", trim($desc));

		$muzzlepart=$member['MuzzleParticle'];

		$anim=$member['Animation'];
		$sound1=$member['Sound_Shot'];
		$sound2=$member['Sound_Reload'];

		$BulletID=$member['BulletID'];
		$Damage=$member['Damage'];
		$isImmediate="true";
		if ($member['isImmediate'] == 0) $isImmediate="false";
		$Mass=$member['Mass'];
		$Speed=$member['Speed'];
		$DamageDecay=$member['DamageDecay'];
		$Area=$member['Area'];
		$Delay=$member['Delay'];
		$Timeout=$member['Timeout'];
		$NumClips=$member['NumClips'];
		$Clipsize=$member['Clipsize'];
		$ReloadTime=$member['ReloadTime'];
		$ActiveReloadTick=$member['ActiveReloadTick'];
		$RateOfFire=$member['RateOfFire'];
		$Spread=$member['Spread'];
		$Recoil=$member['Recoil'];
		$Weight=$member['Weight'];

		$GR1=$member['NumGrenades'];
		$GR2=$member['GrenadeName'];

		$Scope=$member['ScopeType'];
		$ScopeZoom=$member['ScopeZoom'];

		$FM=$member['Firemode'];

		$LevelRequired=$member['LevelRequired'];
		$IsUpgradeable=$member['IsUpgradeable'];

		list($xx, $yy, $zz) = sscanf($member['MuzzleOffset'], "%f %f %f");

		$AnimPrefix = $member['AnimPrefix'];

		$xml_output .= "\t<Weapon itemID=\"" . $itemid . "\" category=\"" . $cat . "\" upgrade=\"" . $IsUpgradeable . "\" FNAME=\"" . $fname . "\" Weight=\"".$Weight."\" >\n ";
		$xml_output .= "\t<Model file=\"Data/ObjectsDepot/Weapons/" . $fname . ".sco\" AnimPrefix=\"" . $AnimPrefix . "\" muzzlerOffset.x=\"".$xx."\" muzzlerOffset.y=\"".$yy."\" muzzlerOffset.z=\"".$zz."\"/>\n";
		$xml_output .= "\t<MuzzleModel file=\"". $muzzlepart . "\" />\n";
		$xml_output .= "\t<HudIcon file=\"\$Data/Weapons/HudIcons/" . $fname . ".dds\"/> \n";
		$xml_output .= "\t<Store name=\"" . $name . "\" icon=\"\$Data/Weapons/StoreIcons/" . $fname . ".dds\" desc=\"".$desc."\" LevelRequired=\"".$LevelRequired."\"/>\n";
   		$xml_output .= "\t<PrimaryFire bullet=\"".$BulletID."\" damage=\"".$Damage."\" immediate=\"".$isImmediate."\" mass=\"".$Mass."\" decay=\"".$DamageDecay."\" speed=\"".$Speed."\" area=\"".$Area."\" delay=\"".$Delay."\" timeout=\"".$Timeout."\" numShells=\"".$NumClips."\" clipSize=\"".$Clipsize."\" reloadTime=\"".$ReloadTime."\" activeReloadTick=\"".$ActiveReloadTick."\" rateOfFire=\"".$RateOfFire."\" spread=\"".$Spread."\" recoil=\"".$Recoil."\" numgrenades=\"".$GR1."\" grenadename=\"".$GR2."\" firemode=\"".$FM."\"  ScopeType=\"".$Scope."\"  ScopeZoom=\"".$ScopeZoom."\" />\n";
		$xml_output .= "\t<Animation type=\"" .$anim."\"/>\n";
		$xml_output .= "\t<Sound shoot=\"" .$sound1."\" reload=\"".$sound2."\"/>\n";
		$xml_output .= "\t<FPS";
		$IsFPS = $member['IsFPS'];
		if($IsFPS > 0)
		{
			xml_addvar('IsFPS', 'IsFPS');
			xml_addvar('i0', 'FPSSpec0');
			xml_addvar('i1', 'FPSSpec1');
			xml_addvar('i2', 'FPSSpec2');
			xml_addvar('i3', 'FPSSpec3');
			xml_addvar('i4', 'FPSSpec4');
			xml_addvar('i5', 'FPSSpec5');
			xml_addvar('i6', 'FPSSpec6');
			xml_addvar('i7', 'FPSSpec7');
			xml_addvar('i8', 'FPSSpec8');

			xml_addvar('d0', 'FPSAttach0');
			xml_addvar('d1', 'FPSAttach1');
			xml_addvar('d2', 'FPSAttach2');
			xml_addvar('d3', 'FPSAttach3');
			xml_addvar('d4', 'FPSAttach4');
			xml_addvar('d5', 'FPSAttach5');
			xml_addvar('d6', 'FPSAttach6');
			xml_addvar('d7', 'FPSAttach7');
			xml_addvar('d8', 'FPSAttach8');
		}
		$xml_output .= "/>\n";

		out_durability();
		out_disassembly();

	  	$xml_output .= "\t</Weapon>\n";
	}
	$xml_output .= "</WeaponsArmory>\n\n";

	// create & execute query: food\water\medical
	$tsql   = "select * from Items_Weapons where Category=30 OR Category=32 OR Category=33";
	$params = array();
	$stmt   = sqlsrv_query($conn, $tsql, $params);

	$xml_output .= "<FoodArmory>\n";
	while($member = sqlsrv_fetch_array($stmt, SQLSRV_FETCH_ASSOC))
	{
		$itemid=$member['ItemID'];
		$fname=str_replace (" ", "",$member['FNAME']);
		$cat=$member['Category'];
		$name=htmlentities($member['Name'], ENT_COMPAT, 'UTF-8');
		$desc=htmlentities($member['Description'], ENT_COMPAT, 'UTF-8');
		$desc=str_replace("\r\n", "&#xD;", trim($desc));

		$Health=$member['ReloadTime'];
		$Tocixity=$member['ActiveReloadTick'];
		$Water=$member['Spread'];
		$Food=$member['Recoil'];
		$Stamina=$member['Area'];

		$shopStackSize=$member['Clipsize'];

		$Weight=$member['Weight'];
		$LevelRequired=$member['LevelRequired'];

		$xml_output .= "\t<Item itemID=\"" . $itemid . "\" category=\"" . $cat . "\" Weight=\"".$Weight."\" >\n";
		$xml_output .= "\t<Model file=\"Data/ObjectsDepot/Weapons/" . $fname . ".sco\" />\n";
   		$xml_output .= "\t<Store name=\"" . $name . "\" icon=\"\$Data/Weapons/StoreIcons/" . $fname . ".dds\" desc=\"".$desc."\" LevelRequired=\"".$LevelRequired."\"/>\n";
   		$xml_output .= "\t<Property health=\"".$Health."\" toxicity=\"".$Tocixity."\" water=\"".$Water."\" food=\"".$Food."\" stamina=\"".$Stamina."\" shopSS=\"".$shopStackSize."\" />\n";

   		out_disassembly();

	  	$xml_output .= "\t</Item>\n";
	}
	$xml_output .= "</FoodArmory>\n\n";

	// create & execute query: vehicle info
	$tsql   = "select * from Items_Weapons where Category=55";
	$params = array();
	$stmt   = sqlsrv_query($conn, $tsql, $params);

	$xml_output .= "<VehicleArmory>\n";
	while($member = sqlsrv_fetch_array($stmt, SQLSRV_FETCH_ASSOC))
	{
		$itemid=$member['ItemID'];
		$fname=str_replace (" ", "",$member['FNAME']);
		$cat=$member['Category'];
		$name=htmlentities($member['Name'], ENT_COMPAT, 'UTF-8');
		$desc=htmlentities($member['Description'], ENT_COMPAT, 'UTF-8');
		$desc=str_replace("\r\n", "&#xD;", trim($desc));

		$durability=$member['ReloadTime'];
		$armor=$member['ActiveReloadTick'];
		$torque=$member['Spread'];
		$omega=$member['Recoil'];
		$fuel=$member['Area'];
		$Weight=$member['Weight'];

		$xml_output .= "\t<Item itemID=\"" . $itemid . "\" category=\"" . $cat . "\" Weight=\"".$Weight."\" >\n";
   		$xml_output .= "\t<Store name=\"" . $name . "\" icon=\"\$Data/Weapons/StoreIcons/" . $fname . ".dds\" desc=\"".$desc."\"/>\n";
   		$xml_output .= "\t<Property durability=\"".$durability."\" armor=\"".$armor."\" torque=\"".$torque."\" omega=\"".$omega."\" fuel=\"".$fuel."\" fname=\"".$fname."\" />\n";

   		out_disassembly();

	  	$xml_output .= "\t</Item>\n";
	}
	$xml_output .= "</VehicleArmory>\n\n";

	// create & execute query: craft components
	$tsql   = "select * from Items_Generic where Category=50";
	$params = array();
	$stmt   = sqlsrv_query($conn, $tsql, $params);

	$xml_output .= "<CraftComponentsArmory>\n";
	while($member = sqlsrv_fetch_array($stmt, SQLSRV_FETCH_ASSOC))
	{
		$itemid=$member['ItemID'];
		$fname=str_replace (" ", "",$member['FNAME']);
		$cat=$member['Category'];
		$name=htmlentities($member['Name'], ENT_COMPAT, 'UTF-8');
		$desc=htmlentities($member['Description'], ENT_COMPAT, 'UTF-8');
		$desc=str_replace("\r\n", "&#xD;", trim($desc));

		$Weight=$member['Weight'];

		$xml_output .= "\t<Item itemID=\"" . $itemid . "\" category=\"" . $cat . "\" Weight=\"".$Weight."\" >\n";
		$xml_output .= "\t<Model file=\"Data/ObjectsDepot/Weapons/" . $fname . ".sco\" />\n";
   		$xml_output .= "\t<Store name=\"" . $name . "\" icon=\"\$Data/Weapons/StoreIcons/" . $fname . ".dds\" desc=\"".$desc."\" LevelRequired=\"".$LevelRequired."\"/>\n";

   		out_disassembly();

	  	$xml_output .= "\t</Item>\n";
	}
	$xml_output .= "</CraftComponentsArmory>\n\n";

	// create & execute query: craft recipes
	$tsql   = "select * from Items_Generic where Category=51";
	$params = array();
	$stmt   = sqlsrv_query($conn, $tsql, $params);

	$xml_output .= "<CraftRecipeArmory>\n";
	while($member = sqlsrv_fetch_array($stmt, SQLSRV_FETCH_ASSOC))
	{
		$itemid=$member['ItemID'];
		$fname=str_replace (" ", "",$member['FNAME']);
		$cat=$member['Category'];
		$name=htmlentities($member['Name'], ENT_COMPAT, 'UTF-8');
		$desc=htmlentities($member['Description'], ENT_COMPAT, 'UTF-8');
		$desc=str_replace("\r\n", "&#xD;", trim($desc));

		$Weight=$member['Weight'];

		$xml_output .= "\t<Item itemID=\"" . $itemid . "\" category=\"" . $cat . "\" Weight=\"".$Weight."\" >\n";
		$xml_output .= "\t<Model file=\"Data/ObjectsDepot/Weapons/" . $fname . ".sco\" />\n";
   		$xml_output .= "\t<Store name=\"" . $name . "\" icon=\"\$Data/Weapons/StoreIcons/" . $fname . ".dds\" desc=\"".$desc."\" LevelRequired=\"".$LevelRequired."\"/>\n";

	  	$xml_output .= "\t</Item>\n";
	}
	$xml_output .= "</CraftRecipeArmory>\n\n";

	// create & execute query: GEARS
	$tsql   = "select * from Items_Gear where Category!=16 AND Category!=12";
	$params = array();
	$stmt   = sqlsrv_query($conn, $tsql, $params);

	$xml_output .= "<GearArmory>\n\n";
	while($member = sqlsrv_fetch_array($stmt, SQLSRV_FETCH_ASSOC))
	{
		$itemid=$member['ItemID'];
		$fname=str_replace (" ", "",$member['FNAME']);
		$cat=$member['Category'];
		$name=htmlentities($member['Name'], ENT_COMPAT, 'UTF-8');
		$desc=htmlentities($member['Description'], ENT_COMPAT, 'UTF-8');
		$desc=str_replace("\r\n", "&#xD;", trim($desc));

		$DmgPerc=$member['DamagePerc'];
		$DmgMax=$member['DamageMax'];
		$Weight=$member['Weight'];
		$Bulkiness=$member['Bulkiness'];
		$Inaccuracy=$member['Inaccuracy'];
		$Stealth=$member['Stealth'];
		$LevelRequired=$member['LevelRequired'];
		$ProtectionLevel=$member['ProtectionLevel'];

		$xml_output .= "\t<Gear itemID=\"" . $itemid . "\" category=\"" . $cat . "\"  Weight=\"".$Weight."\" >\n ";
		$xml_output .= "\t<Model file=\"Data/ObjectsDepot/Characters/" . $fname . ".sco\" />\n";
		$xml_output .= "\t<Store name=\"" . $name . "\" icon=\"\$Data/Weapons/StoreIcons/" . $fname . ".dds\" desc=\"".$desc."\" LevelRequired=\"".$LevelRequired."\" />\n";
		$xml_output .= "\t<Armor damagePerc=\"" . $DmgPerc . "\" damageMax=\"" . $DmgMax . "\" bulkiness=\"" . $Bulkiness . "\" inaccuracy=\"" . $Inaccuracy . "\" stealth=\"" . $Stealth . "\" ProtectionLevel=\"".$ProtectionLevel."\" />\n";

		out_durability();
		out_disassembly();

	  	$xml_output .= "\t</Gear>\n";
	}
	$xml_output .= "</GearArmory>\n\n";

	// create & execute query: HERO PACKAGES
	$tsql   = "select * from Items_Gear where Category=16";
	$params = array();
	$stmt   = sqlsrv_query($conn, $tsql, $params);

	$xml_output .= "<HeroArmory>\n\n";
	while($member = sqlsrv_fetch_array($stmt, SQLSRV_FETCH_ASSOC))
	{
		$itemid=$member['ItemID'];
		$fname=str_replace (" ", "",$member['FNAME']);
		$cat=$member['Category'];
		$name=htmlentities($member['Name'], ENT_COMPAT, 'UTF-8');
		$desc=htmlentities($member['Description'], ENT_COMPAT, 'UTF-8');
		$desc=str_replace("\r\n", "&#xD;", trim($desc));

		$DmgPerc=$member['DamagePerc'];
		$DmgMax=$member['DamageMax'];
		$Weight=$member['Weight'];
		$MaxHeads=$member['Bulkiness'];
		$MaxBodys=$member['Inaccuracy'];
		$MaxLegs=$member['Stealth'];
		$LevelRequired=$member['LevelRequired'];
		$ProtectionLevel=$member['ProtectionLevel'];

    	$xml_output .= "\t<Hero itemID=\"" . $itemid . "\" category=\"" . $cat . "\" Weight=\"".$Weight."\" >\n ";
    	$xml_output .= "\t<Model file=\"Data/ObjectsDepot/Characters/" . $fname . "\" />\n";
		$xml_output .= "\t<Store name=\"" . $name . "\" icon=\"\$Data/Weapons/StoreIcons/" . $fname . ".dds\" desc=\"".$desc."\" LevelRequired=\"".$LevelRequired."\" />\n";
		$xml_output .= "\t<HeroDesc damagePerc=\"" . $DmgPerc . "\" damageMax=\"" . $DmgMax . "\" maxHeads=\"" . $MaxHeads . "\" maxBodys=\"" . $MaxBodys . "\" maxLegs=\"" . $MaxLegs . "\" ProtectionLevel=\"".$ProtectionLevel."\" />\n";
	  	$xml_output .= "\t</Hero>\n";
	}
	$xml_output .= "</HeroArmory>\n\n";

	// create & execute query: Backpacks
	$tsql   = "select * from Items_Gear where Category=12";
	$params = array();
	$stmt   = sqlsrv_query($conn, $tsql, $params);

	$xml_output .= "<BackpackArmory>\n\n";
	while($member = sqlsrv_fetch_array($stmt, SQLSRV_FETCH_ASSOC))
	{
		$itemid=$member['ItemID'];
		$fname=str_replace (" ", "",$member['FNAME']);
		$cat=$member['Category'];
		$name=htmlentities($member['Name'], ENT_COMPAT, 'UTF-8');
		$desc=htmlentities($member['Description'], ENT_COMPAT, 'UTF-8');
		$desc=str_replace("\r\n", "&#xD;", trim($desc));

		$MaxSlots=$member['Bulkiness'];
		$MaxWeight=$member['Inaccuracy'];
		$Weight=$member['Weight'];

	    $xml_output .= "\t<Backpack itemID=\"" . $itemid . "\" category=\"" . $cat . "\" Weight=\"".$Weight."\" >\n ";
	    $xml_output .= "\t<Model file=\"Data/ObjectsDepot/Characters/" . $fname . ".sco\" />\n";
		$xml_output .= "\t<Store name=\"" . $name . "\" icon=\"\$Data/Weapons/StoreIcons/" . $fname . ".dds\" desc=\"".$desc."\" />\n";
		$xml_output .= "\t<Desc maxSlots=\"" . $MaxSlots . "\" maxWeight=\"" . $MaxWeight . "\" />\n";
	  	$xml_output .= "\t</Backpack>\n";
	}
	$xml_output .= "</BackpackArmory>\n\n";

	// create & execute query: ITEMS
	$tsql   = "select * from Items_Generic where Category!=7 and Category!=50 and Category!=51"; // skip lootboxes and craft components + recipes
	$params = array();
	$stmt   = sqlsrv_query($conn, $tsql, $params);

	$xml_output .= "<ItemsDB>\n\n";
	while($member = sqlsrv_fetch_array($stmt, SQLSRV_FETCH_ASSOC))
	{
		$itemid=$member['ItemID'];
		$fname=str_replace (" ", "",$member['FNAME']);
		$cat=$member['Category'];
		$name=htmlentities($member['Name'], ENT_COMPAT, 'UTF-8');
		$desc=htmlentities($member['Description'], ENT_COMPAT, 'UTF-8');
		$desc=str_replace("\r\n", "&#xD;", trim($desc));

		$LevelRequired=$member['LevelRequired'];
		$Weight=$member['Weight'];

		$xml_output .= "\t<Item itemID=\"" . $itemid . "\" category=\"" . $cat . "\" Weight=\"".$Weight."\" >\n ";
		if($cat == 4) // storecat_Items
		{
			$xml_output .= "\t<Model file=\"Data/ObjectsDepot/Weapons/" . $fname . ".sco\" />\n";
		}
		$xml_output .= "\t<Store name=\"" . $name . "\" icon=\"\$Data/Weapons/StoreIcons/" . $fname . ".dds\" desc=\"".$desc."\" LevelRequired=\"".$LevelRequired."\" />\n";

		out_disassembly();

	  	$xml_output .= "\t</Item>\n";
	}
	$xml_output .= "</ItemsDB>\n\n";

	// create & execute query: Attachments
	$tsql   = "select * from Items_Attachments";
	$params = array();
	$stmt   = sqlsrv_query($conn, $tsql, $params);

	$xml_output .= "<AttachmentArmory>\n";
	while($member = sqlsrv_fetch_array($stmt, SQLSRV_FETCH_ASSOC))
	{
		$itemid=$member['ItemID'];
		$fname=str_replace (" ", "",$member['FNAME']);
		$type=$member['Type'];
		$name=htmlentities($member['Name'], ENT_COMPAT, 'UTF-8');
		$desc=htmlentities($member['Description'], ENT_COMPAT, 'UTF-8');
		$desc=str_replace("\r\n", "&#xD;", trim($desc));

		$MuzzleParticle=$member['MuzzleParticle'];
		$FireSound=$member['FireSound'];
		$Damage=$member['Damage'];
		$Range=$member['Range'];
		$Firerate=$member['Firerate'];
		$Recoil=$member['Recoil'];
		$Spread=$member['Spread'];
		$Clipsize=$member['Clipsize'];
		$ScopeMag=$member['ScopeMag'];
		$ScopeType=$member['ScopeType'];
		$ScopeAnimPrefix=$member['AnimPrefix'];
		$SpecID=$member['SpecID'];

		$LevelRequired=$member['LevelRequired'];
		$Weight=$member['Weight'];

		$cat=19; // hard coded

		$xml_output .= "\t<Attachment itemID=\"" . $itemid . "\" category=\"" . $cat . "\" type=\"" . $type . "\" SpecID=\"" . $SpecID . "\" Weight=\"".$Weight."\" >\n ";
		$xml_output .= "\t<Model file=\"Data/ObjectsDepot/Weapons/" . $fname . ".sco\" MuzzleParticle=\"" .$MuzzleParticle. "\" FireSound=\"".$FireSound."\" ScopeAnim=\"".$ScopeAnimPrefix."\" />\n";
		$xml_output .= "\t<Store name=\"" . $name . "\" icon=\"\$Data/Weapons/StoreIcons/" . $fname . ".dds\" desc=\"".$desc."\" LevelRequired=\"".$LevelRequired."\" />\n";
		$xml_output .= "\t<Upgrade damage=\"" . $Damage . "\" range=\"" . $Range . "\" firerate=\"" . $Firerate . "\" recoil=\"" . $Recoil . "\" spread=\"" . $Spread . "\" clipsize=\"" . $Clipsize . "\" ScopeMag=\"".$ScopeMag."\" ScopeType=\"".$ScopeType."\" />\n";

		out_durability();
		out_disassembly();

	  	$xml_output .= "\t</Attachment>\n";
	}
	$xml_output .= "</AttachmentArmory>\n\n";

	// create & execute query: LOOTBOXES
	$tsql   = "select * from Items_Generic where Category=7";
	$params = array();
	$stmt   = sqlsrv_query($conn, $tsql, $params);

	$xml_output .= "<LootBoxDB>\n\n";
	while($member = sqlsrv_fetch_array($stmt, SQLSRV_FETCH_ASSOC))
	{
		$itemid=$member['ItemID'];
		$fname=str_replace (" ", "",$member['FNAME']);
		$cat=$member['Category'];
		$name=htmlentities($member['Name'], ENT_COMPAT, 'UTF-8');
		$desc=htmlentities($member['Description'], ENT_COMPAT, 'UTF-8');
		$desc=str_replace("\r\n", "&#xD;", trim($desc));

		$LevelRequired=$member['LevelRequired'];
		$Weight=0;

		$xml_output .= "\t<LootBox itemID=\"" . $itemid . "\" category=\"" . $cat . "\" Weight=\"".$Weight."\" >\n ";
		$xml_output .= "\t<Model file=\"Data/ObjectsDepot/Weapons/" . $fname . ".sco\" />\n";
		$xml_output .= "\t<Store name=\"" . $name . "\" icon=\"\$Data/Weapons/StoreIcons/" . $fname . ".dds\" desc=\"".$desc."\" LevelRequired=\"".$LevelRequired."\" />\n";

		if($ServerMode == 2)
		{
			$tsql2   = "select * from Items_LootData where LootID=$itemid order by Chance asc";
			$params2 = array();
			$stmt2   = sqlsrv_query($conn, $tsql2, $params2);

	  		$xml_output .= "\t<data>\n";
			while($member2 = sqlsrv_fetch_array($stmt2, SQLSRV_FETCH_ASSOC))
			{
				$l_Chance = $member2['Chance'];
				$l_ItemID = $member2['ItemID'];
				$l_GDMin  = $member2['GDMin'];
				$l_GDMax  = $member2['GDMax'];

				$xml_output .= "\t\t<d c=\"$l_Chance\" i=\"$l_ItemID\" g1=\"$l_GDMin\" g2=\"$l_GDMax\" />\n";
			}
	  		$xml_output .= "\t</data>\n";
		}

	  	$xml_output .= "\t</LootBox>\n";
	}
	$xml_output .= "</LootBoxDB>\n\n";


	$xml_output .= "</DB>\n\n";

	echo $xml_output;
	exit();
?>