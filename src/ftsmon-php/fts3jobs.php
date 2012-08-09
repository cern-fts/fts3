<?php
session_start();

$config = parse_ini_file('fts3config.ini', 1);


// store session data
$_SESSION['user']=$config['db']['user'];
$_SESSION['pass']=$config['db']['pass'];
$_SESSION['connstring']=$config['db']['connstring'];
?>
<html>
<head>
<title>FTS 3 Pilot service</title>
<META http-equiv="refresh" content="200">
<style type="text/css">
<!--
@import url("style.css");
-->
</style>
</head>
<body>

<b>FTS 3 Pilot service - <a href="fts3config.php">Configuration</a> - (updated every 200 seconds / last 12h jobs fetched) </b>

<form name="frmSearch" method="get" action="fts3jobs.php">
<table width="599" border="1">
<tr>
<th>JOB ID
<input name="jobid" type="text" id="jobid" size="50" value="<?php $_GET['jobid'];?>">
<input type="submit" value="Search"></th>
</tr>
</table>
</form>

<?php
$vo = $_GET['vo'];
$selected="selected=true";
?>

<form name="frmSearch2" method="get" action="fts3jobs.php">
<table width="599" border="1">
<tr>
<th>VO
<select name="vo" onChange="javascript:this.form.submit();">
<?php
if($vo=="all"){
?>
<option value="all" selected=true>All</option>
<?php
}else{
?>
<option value="all" >All</option>
<?php
}
?>
<?php
if($vo=="atlas"){
?>
<option value="atlas" selected=true>atlas</option>
<?php
}else{
?>
<option value="atlas" >atlas</option>
<?php
}
?>
<?php
if($vo=="cms"){
?>
<option value="cms" selected=true>cms</option>
<?php
}else{
?>
<option value="cms" >cms</option>
<?php
}
?>
<?php
if($vo=="lhcb"){
?>
<option value="lhcb" selected=true>lhcb</option>
<?php
}else{
?>
<option value="lhcb" >lhcb</option>
<?php
}
?>
<?php
if($vo=="dteam"){
?>
<option value="dteam" selected=true>dteam</option>
<?php
}else{
?>
<option value="dteam" >dteam</option>
<?php
}
?>
<input type="submit" value="Search"></th>
</tr>
</table>
</form>

<?php
$jobid = $_GET['jobid'];


if($jobid != ""){
	$strSQL = "SELECT  JOB_ID, VO_NAME, SOURCE_SE, DEST_SE, JOB_STATE, SUBMIT_TIME FROM T_JOB where JOB_ID='$jobid' ORDER BY SYS_EXTRACT_UTC(t_job.SUBMIT_TIME) ";
}
elseif($vo != "" && $vo!="all"){
	$strSQL = "SELECT  JOB_ID, VO_NAME, SOURCE_SE, DEST_SE, JOB_STATE, SUBMIT_TIME FROM T_JOB where VO_NAME='$vo' and (SUBMIT_TIME > (CURRENT_TIMESTAMP - interval '12' hour)) ORDER BY SYS_EXTRACT_UTC(t_job.SUBMIT_TIME) ";
}
else{
	$strSQL = "SELECT  JOB_ID, VO_NAME, SOURCE_SE, DEST_SE, JOB_STATE, SUBMIT_TIME FROM T_JOB where (SUBMIT_TIME > (CURRENT_TIMESTAMP - interval '12' hour))  ORDER BY SYS_EXTRACT_UTC(t_job.SUBMIT_TIME) ";
}


$objConnect = oci_connect($_SESSION['user'],$_SESSION['pass'],$_SESSION['connstring']);
$objParse = oci_parse ($objConnect, $strSQL);
oci_execute ($objParse);

?>
<table width="100%" border="1">
<tr>
<th > <div align="center">JOB ID </div></th>
<th > <div align="center">VO </div></th>
<th > <div align="center">SOURCE SE </div></th>
<th > <div align="center">DEST SE </div></th>
<th > <div align="center">JOB STATE </div></th>
<th > <div align="center">SUBMIT TIME </div></th>

</tr>
<?php
while($objResult = oci_fetch_array($objParse,OCI_BOTH))
{

if( $objResult["JOB_STATE"] == "FINISHED"){
?>
<tr >
<td><a href="fts3files.php?jobid=<?php echo $objResult["JOB_ID"];?>"><?php echo $objResult["JOB_ID"];?></a></td>
<td><?php echo $objResult["VO_NAME"];?></td>
<td><?php echo $objResult["SOURCE_SE"];?></td>
<td><?php echo $objResult["DEST_SE"];?></td>
<td bgcolor="#00FF00"><?php echo $objResult["JOB_STATE"];?></td>
<td><?php echo $objResult["SUBMIT_TIME"];?></td>
</tr>
<?php
}
if( $objResult["JOB_STATE"] == "FAILED"){
?>
<tr >
<td><a href="fts3files.php?jobid=<?php echo $objResult["JOB_ID"];?>"><?php echo $objResult["JOB_ID"];?></a></td>
<td><?php echo $objResult["VO_NAME"];?></td>
<td><?php echo $objResult["SOURCE_SE"];?></td>
<td><?php echo $objResult["DEST_SE"];?></td>
<td bgcolor="red"><?php echo $objResult["JOB_STATE"];?></td>
<td><?php echo $objResult["SUBMIT_TIME"];?></td>
</tr>
<?php
}
if( $objResult["JOB_STATE"] == "ACTIVE"){
?>
<tr >
<td><a href="fts3files.php?jobid=<?php echo $objResult["JOB_ID"];?>"><?php echo $objResult["JOB_ID"];?></a></td>
<td><?php echo $objResult["VO_NAME"];?></td>
<td><?php echo $objResult["SOURCE_SE"];?></td>
<td><?php echo $objResult["DEST_SE"];?></td>
<td bgcolor="#00FFFF"><?php echo $objResult["JOB_STATE"];?></td>
<td><?php echo $objResult["SUBMIT_TIME"];?></td>
</tr>
<?php
}
if( $objResult["JOB_STATE"] == "FINISHEDDIRTY"){
?>
<tr >
<td><a href="fts3files.php?jobid=<?php echo $objResult["JOB_ID"];?>"><?php echo $objResult["JOB_ID"];?></a></td>
<td><?php echo $objResult["VO_NAME"];?></td>
<td><?php echo $objResult["SOURCE_SE"];?></td>
<td><?php echo $objResult["DEST_SE"];?></td>
<td bgcolor="yellow"><?php echo $objResult["JOB_STATE"];?></td>
<td><?php echo $objResult["SUBMIT_TIME"];?></td>
</tr>
<?php
}
if( $objResult["JOB_STATE"] == "CANCELED"){
?>
<tr >
<td><a href="fts3files.php?jobid=<?php echo $objResult["JOB_ID"];?>"><?php echo $objResult["JOB_ID"];?></a></td>
<td><?php echo $objResult["VO_NAME"];?></td>
<td><?php echo $objResult["SOURCE_SE"];?></td>
<td><?php echo $objResult["DEST_SE"];?></td>
<td bgcolor="#FF0000"><?php echo $objResult["JOB_STATE"];?></td>
<td><?php echo $objResult["SUBMIT_TIME"];?></td>
</tr>
<?php
}
}
?>
</table>
<?php
oci_close($objConnect);
?>
</body>
</html>
