<?php
session_start();
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

<b>FTS 3 Pilot service  - Queued jobs</b>

<form name="frmSearch" method="get" action="fts3queue.php">
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

<form name="frmSearch2" method="get" action="fts3queue.php">
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
	$strSQL = "SELECT  JOB_ID, VO_NAME, SOURCE_SE, DEST_SE, SUBMIT_TIME,  SOURCE_SPACE_TOKEN,SPACE_TOKEN FROM T_JOB where JOB_ID='$jobid' AND JOB_STATE='SUBMITTED' ORDER BY SYS_EXTRACT_UTC(t_job.SUBMIT_TIME) DESC ";
}
elseif($vo != "" && $vo!="all"){
	$strSQL = "SELECT  JOB_ID, VO_NAME, SOURCE_SE, DEST_SE, SUBMIT_TIME, SOURCE_SPACE_TOKEN,SPACE_TOKEN FROM T_JOB where VO_NAME='$vo' and JOB_STATE='SUBMITTED' and (SUBMIT_TIME > (CURRENT_TIMESTAMP - interval '12' hour)) ORDER BY SYS_EXTRACT_UTC(t_job.SUBMIT_TIME) DESC ";
}
else{
	$strSQL = "SELECT  JOB_ID, VO_NAME, SOURCE_SE, DEST_SE, SUBMIT_TIME, SOURCE_SPACE_TOKEN,SPACE_TOKEN FROM T_JOB where  JOB_STATE='SUBMITTED'  AND (SUBMIT_TIME > (CURRENT_TIMESTAMP - interval '12' hour))  ORDER BY SYS_EXTRACT_UTC(t_job.SUBMIT_TIME) DESC ";
}

try {
   $conn = new PDO("oci:dbname=".$_SESSION['connstring'],$_SESSION['user'],$_SESSION['pass']);
}
catch(PDOException $e) {
   die("Could not connect to the database\n");
}

$stmt = $conn->query($strSQL);




?>

<a href="fts3jobs.php?jobid="> Back to job listing</a>

<table width="100%" border="1">
<tr>
<th > <div align="center">JOB ID </div></th>
<th > <div align="center">VO </div></th>
<th > <div align="center">SUBMIT TIME </div></th>
</tr>
<?php
while($objResult = $stmt->fetch(PDO::FETCH_BOTH))
{

$job2 = $objResult["JOB_ID"];

$strSQL2 = "select SOURCE_SURL, DEST_SURL from t_file where job_id='$job2' and file_state='SUBMITTED'";
$stmt2 = $conn->query($strSQL2);
$line = "";
while($objResult2 = $stmt2->fetch(PDO::FETCH_BOTH)){
	$line .= "    ----> ".$objResult2["SOURCE_SURL"]."   ".$objResult2["DEST_SURL"]."</br>";
}

?>
<tr >
<td><b><?php echo $objResult["JOB_ID"];?><b></br><?php echo $line; ?></td>
<td><?php echo $objResult["VO_NAME"];?></td>
<td><?php echo $objResult["SUBMIT_TIME"];?></td>
</tr>
<?php
}
?>
</table>
<?php
$conn = null;
?>
</body>
</html>
