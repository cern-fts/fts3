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
$jobid = $_GET['jobid'];
$objConnect = oci_connect($_SESSION['user'],$_SESSION['pass'],$_SESSION['connstring']);
$strSQL = "SELECT  JOB_ID, SOURCE_SURL,DEST_SURL,FILE_STATE, REASON, START_TIME, FINISH_TIME  FROM T_FILE WHERE JOB_ID='$jobid' ORDER BY t_file.file_id DESC, SYS_EXTRACT_UTC(t_file.start_time) ";
$objParse = oci_parse ($objConnect, $strSQL);
oci_execute ($objParse);

?>
<table width="100%" border="1">
<tr>
<th > <div align="center">JOB ID </div></th>
<th > <div align="center">SOURCE_SURL </div></th>
<th > <div align="center">DEST_SURL </div></th>
<th > <div align="center">FILE_STATE </div></th>
<th > <div align="center">REASON </div></th>
<th > <div align="center">START TIME </div></th>
<th > <div align="center">FINISH TIME </div></th>

</tr>
<?php
while($objResult = oci_fetch_array($objParse,OCI_BOTH))
{

if( $objResult["FILE_STATE"] == "FINISHED"){
?>
<tr bgcolor="#00FF00">
<td><?php echo $objResult["JOB_ID"];?></td>
<td><?php echo $objResult["SOURCE_SURL"];?></td>
<td><?php echo $objResult["DEST_SURL"];?></td>
<td><?php echo $objResult["FILE_STATE"];?></td>
<td><?php echo $objResult["REASON"];?></td>
<td><?php echo $objResult["START_TIME"];?></td>
<td><?php echo $objResult["FINISH_TIME"];?></td>
</tr>
<?php
}
if( $objResult["FILE_STATE"] == "FAILED"){
?>
<tr bgcolor="red">
<td><?php echo $objResult["JOB_ID"];?></td>
<td><?php echo $objResult["SOURCE_SURL"];?></td>
<td><?php echo $objResult["DEST_SURL"];?></td>
<td><?php echo $objResult["FILE_STATE"];?></td>
<td><?php echo $objResult["REASON"];?></td>
<td><?php echo $objResult["START_TIME"];?></td>
<td><?php echo $objResult["FINISH_TIME"];?></td>
</tr>
<?php
}
if( $objResult["FILE_STATE"] == "ACTIVE"){
?>
<tr bgcolor="yellow">
<td><?php echo $objResult["JOB_ID"];?></td>
<td><?php echo $objResult["SOURCE_SURL"];?></td>
<td><?php echo $objResult["DEST_SURL"];?></td>
<td><?php echo $objResult["FILE_STATE"];?></td>
<td><?php echo $objResult["REASON"];?></td>
<td><?php echo $objResult["START_TIME"];?></td>
<td><?php echo $objResult["FINISH_TIME"];?></td>
</tr>
<?php
}
if( $objResult["FILE_STATE"] == "CANCELED"){
?>
<tr bgcolor="yellow">
<td><?php echo $objResult["JOB_ID"];?></td>
<td><?php echo $objResult["SOURCE_SURL"];?></td>
<td><?php echo $objResult["DEST_SURL"];?></td>
<td><?php echo $objResult["FILE_STATE"];?></td>
<td><?php echo $objResult["REASON"];?></td>
<td><?php echo $objResult["START_TIME"];?></td>
<td><?php echo $objResult["FINISH_TIME"];?></td>
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
