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
$strSQL = "SELECT  INTERNAL_FILE_PARAMS,JOB_ID, SOURCE_SURL,DEST_SURL,FILE_STATE, FILESIZE, REASON, START_TIME, FINISH_TIME, THROUGHPUT, CHECKSUM,TX_DURATION FROM T_FILE WHERE JOB_ID='$jobid' ORDER BY t_file.file_id DESC, SYS_EXTRACT_UTC(t_file.start_time) ";

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
<th > <div align="center">PARAMS </div></th>

</tr>
<?php
$k = 1024;
$m = 1048576;
$g = 1073741824;

function to_bytes($value, $unit)
{
  global $k, $m, $g;
  switch (trim($unit))
  {
    case "b": //bytes
      return ($value / 8);
    case "Kb": //Kilobits
      return (($value * $k) / 8);
    case "Mb": // Megabits
      return (($value * $m) / 8);
    case "Gb": // Gigabits
      return (($value * $g) / 8);
    case "B": // Bytes
      return $value;
    case "KB": // Kilobytes
      return ($value * $k);
    case "MB": // Megabytes
      return ($value * $m);
    case "GB": // Gigabytes
      return ($value * $g);
    default: return 0;
  }
}

function bytes_to($value, $unit)
{
  global $k, $m, $g;
  switch (trim($unit))
  {
    case "b": //bytes
      return ($value * 8);
    case "Kb": //Kilobits
      return (($value * 8) / $k);
    case "Mb": // Megabits
      return (($value * 8) / $m);
    case "Gb": // Gigabits
      return (($value * 8) / $g);
    case "B": // Bytes
      return $value;
    case "KB": // Kilobytes
      return ($value / $k);
      //return bcdiv($value, $k, 4);
    case "MB": // Megabytes
      return ($value / $m);
    case "GB": // Gigabytes
      return ($value / $g);
    default: return 0;
  }
}


# ##############################

$Suffix=array(" b/sec"," kbit/sec"," Mbit/sec"," Gbit/sec"); 

function formatBytes($bytes){ 
	global $Suffix;    
        $i = 0;
        $dblSByte=0;
        if($bytes < 1024){
				
		return (string)(round($bytes,2).$Suffix[0]);	
	}else{
        	for ($i = 0; (int)($bytes / 1024) > 0; $i++, $bytes /= 1024)
            		$dblSByte = $bytes / 1024.0;
	}
	#echo $dblSByte;
	#echo "<br>";
        return (string)(round($dblSByte,2).$Suffix[$i]);
}

function startsWith($haystack,$needle,$case=true) {
    if($case){return (strcmp(substr($haystack, 0, strlen($needle)),$needle)===0);}
    return (strcasecmp(substr($haystack, 0, strlen($needle)),$needle)===0);
    }

while($objResult = $stmt->fetch(PDO::FETCH_BOTH) )
{

if( $objResult["FILE_STATE"] == "FINISHED"){

$fsize = $objResult["FILESIZE"];
$tx_time = $objResult["TX_DURATION"];
$duration = $fsize / $tx_time;
$total = formatBytes($duration);
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
<td><?php echo $total; ?></td>
<td><?php echo $objResult["INTERNAL_FILE_PARAMS"];?></td>
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
<td></td>
<td><?php echo $objResult["INTERNAL_FILE_PARAMS"];?></td>
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
<td> </td>
<td><?php echo $objResult["INTERNAL_FILE_PARAMS"];?></td>
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
<td> </td>
<td><?php echo $objResult["INTERNAL_FILE_PARAMS"];?></td>
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
