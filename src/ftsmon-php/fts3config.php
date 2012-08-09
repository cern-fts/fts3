<?php
session_start();
?>
<html>
<head>
<title>FTS 3 Pilot service</title>
<style type="text/css">
<!--
@import url("style.css");
-->
</style>
</head>
<body>
<a href="fts3jobs.php?jobid="> Back to job listing</a>
<?php

$objConnect = oci_connect($_SESSION['user'],$_SESSION['pass'],$_SESSION['connstring']);
$strSQL = "SELECT  WHEN, DN,CONFIG FROM T_CONFIG_AUDIT where action like '%insert%'";
$objParse = oci_parse ($objConnect, $strSQL);
oci_execute ($objParse);
echo "<table>";
echo "<tr>";
echo "<td>Timeastamp";
echo "</td>";
echo "<td>User DN";
echo "</td>";
echo "<td>Configuration";
echo "</td>";
echo "</tr>";
while($objResult = oci_fetch_array($objParse,OCI_BOTH))
{
 echo "<tr>";
 echo "<td>";
echo  $objResult["WHEN"];
echo "</td>";
echo "<td>";
echo  $objResult["DN"];
echo "</td>";
echo "<td>";
	$config=$objResult["CONFIG"];
        $arr = json_decode($config, TRUE);
 
        foreach($arr as $k=>$v){
                echo "$k = $v";
		echo "<br>";
		foreach($v as $k2=>$v2){
			echo "$k2";
			echo "=";
			echo "$v2";
			echo "<br>";
		}               
        }
 echo "</td>";
 echo "</tr>";


}
        echo "</table>";
?>




</body>
</html>
