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
$strSQL = "SELECT  JOB_ID, SOURCE_SURL,DEST_SURL,FILE_STATE, FILESIZE, REASON, START_TIME, FINISH_TIME, THROUGHPUT, CHECKSUM FROM T_FILE WHERE JOB_ID='$jobid' ORDER BY t_file.file_id DESC, SYS_EXTRACT_UTC(t_file.start_time) ";

try {
   $conn = new PDO("oci:dbname=".$_SESSION['connstring'],$_SESSION['user'],$_SESSION['pass']);
}
catch(PDOException $e) {
   die("Could not connect to the database\n");
}

$stmt = $conn->query($strSQL);

?>
<table width="100%" border="1">
<tr>
<th > <div align="center">JOB ID </div></th>
<th > <div align="center">SOURCE URL </div></th>
<th > <div align="center">DEST URL </div></th>
<th > <div align="center">FILE STATE </div></th>
<th > <div align="center">FILE SIZE </div></th>
<th > <div align="center">CHECKSUM </div></th>
<th > <div align="center">REASON </div></th>
<th > <div align="center">START TIME </div></th>
<th > <div align="center">FINISH TIME </div></th>
<th > <div align="center">THROUGHPUT </div></th>

</tr>
<?php
while($objResult = $stmt->fetch(PDO::FETCH_BOTH) )
{

if( $objResult["FILE_STATE"] == "FINISHED"){
?>
<tr bgcolor="#00FF00">
<td><?php echo $objResult["JOB_ID"];?></td>
<td><?php echo $objResult["SOURCE_SURL"];?></td>
<td><?php echo $objResult["DEST_SURL"];?></td>
<td><?php echo $objResult["FILE_STATE"];?></td>
<td><?php echo $objResult["FILESIZE"];?> B</td>
<td><?php echo $objResult["CHECKSUM"];?></td>
<td><?php echo $objResult["REASON"];?></td>
<td><?php echo $objResult["START_TIME"];?></td>
<td><?php echo $objResult["FINISH_TIME"];?></td>
<td><?php echo $objResult["THROUGHPUT"];?> Mbit/s</td>
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
<td><?php echo $objResult["FILESIZE"];?> B</td>
<td><?php echo $objResult["CHECKSUM"];?></td>
<td><?php echo $objResult["REASON"];?></td>
<td><?php echo $objResult["START_TIME"];?></td>
<td><?php echo $objResult["FINISH_TIME"];?></td>
<td><?php echo $objResult["THROUGHPUT"];?> Mbit/s</td>
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
<td><?php echo $objResult["FILESIZE"];?> B</td>
<td><?php echo $objResult["CHECKSUM"];?></td>
<td><?php echo $objResult["REASON"];?></td>
<td><?php echo $objResult["START_TIME"];?></td>
<td><?php echo $objResult["FINISH_TIME"];?></td>
<td><?php echo $objResult["THROUGHPUT"];?> Mbit/s</td>
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
<td><?php echo $objResult["FILESIZE"];?> B</td>
<td><?php echo $objResult["CHECKSUM"];?></td>
<td><?php echo $objResult["REASON"];?></td>
<td><?php echo $objResult["START_TIME"];?></td>
<td><?php echo $objResult["FINISH_TIME"];?></td>
<td><?php echo $objResult["THROUGHPUT"];?> Mbit/s</td>
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
