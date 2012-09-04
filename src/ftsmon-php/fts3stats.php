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

<b>FTS 3 Pilot service  - Statistics (last 12h jobs fetched)</b></br></br>
<b>Results are valid ONLY when Optimizer is enabled</b>

<?php
$activeTotal = "select count(*) from t_file where file_state='ACTIVE'";
$distinctVO = "select distinct vo_name from t_job";


try {
   $conn = new PDO("oci:dbname=".$_SESSION['connstring'],$_SESSION['user'],$_SESSION['pass']);
}
catch(PDOException $e) {
   die("Could not connect to the database\n");
}

?>
</br></br>
<a href="fts3jobs.php?jobid="> Back to job listing</a>

<table width="100%" border="1">

<?php
#count all active
$stmt1 = $conn->prepare($activeTotal);
$stmt1->execute();
$result = $stmt1->fetchColumn();

#count all queued
$stmt11 = $conn->prepare("select count(*) from t_file where file_state='READY' or file_state='SUBMITTED'");
$stmt11->execute();
$queued = $stmt11->fetchColumn();


#active per VO
$stmt2 = $conn->query($distinctVO);
while($objResult = $stmt2->fetch(PDO::FETCH_BOTH))
{
	$voName = $objResult["VO_NAME"];
	$totalActivePerVo = "select count(*) from t_file,t_job where t_file.job_id = t_job.job_id and t_job.vo_name='$voName' and file_state='ACTIVE'";
	$stmt3 = $conn->prepare($totalActivePerVo);
	$stmt3 ->execute();
	$line = "";
	$actPerVo = $stmt3->fetchColumn();
	$line .= $voName.": ".$actPerVo."</br>";	
}

$distinctSe = "select distinct source_se,dest_se from t_optimize where throughput is not null and (when > (CURRENT_TIMESTAMP - interval '12' hour))";
$stmt4 = $conn ->query($distinctSe);
$avgThr = "";
$avgDurPerSePairxx = "";
while($objResult4 = $stmt4->fetch(PDO::FETCH_BOTH))
{
	$source = $objResult4["SOURCE_SE"];
	$dest = $objResult4["DEST_SE"];	
	$totalActivePerSePair = "select avg(throughput) from t_optimize where source_se='$source' and dest_se='$dest' and throughput is not null and (when > (CURRENT_TIMESTAMP - interval '12' hour)) ";
	$stmt5 = $conn->prepare($totalActivePerSePair);
	$stmt5->execute();
	$actPerSePair = $stmt5->fetchColumn();
	$avgThr .= $source." -> ".$dest." : ".round($actPerSePair,2)." Mbit/sec</br>";
	
	$totalDurationPerSePair = "select avg(TX_DURATION) from t_file where source_surl like '%$source%' and dest_surl like '%$dest%' and TX_DURATION is not null and (FINISH_TIME > (CURRENT_TIMESTAMP - interval '12' hour))";
	$stmt6 = $conn->prepare($totalDurationPerSePair);
	$stmt6->execute();
	$avgDurPerSePair = $stmt6->fetchColumn();
	$avgDurPerSePairxx .= $source." -> ".$dest." : ".round($avgDurPerSePair,2)." secs</br>";	
}

$reasonFailures = "select distinct reason from t_file where reason is not null order by reason asc";
$stmt7 = $conn ->query($reasonFailures);
$reason = "";
while($objResult7 = $stmt7->fetch(PDO::FETCH_BOTH))
{
	$testE = $objResult7["REASON"];
	$countReason = "select count(*) from t_file where reason='$testE' ";
	$stmt33 = $conn->prepare($countReason);
	$stmt33 ->execute();
	$countR = $stmt33->fetchColumn();	
	$reason .=$countR." : ".$objResult7["REASON"]."</br></br>";
}



#success rate
$success = "select count(*) from t_file where file_state='FINISHED' ";
$stmt13 = $conn->prepare($success);
$stmt13 ->execute();
$succ = $stmt13->fetchColumn();

#failure rate
$failed = "select count(*) from t_file where file_state!='FINISHED' ";
$stmt14 = $conn->prepare($failed);
$stmt14 ->execute();
$fail = $stmt14->fetchColumn();

$ratioSuccessFailure = round(($succ/($succ + $fail)) * (100/1),1);

?>

<tr><td>SUCCESS RATE</td><td><?php echo $ratioSuccessFailure;?>%</td></tr>
<tr><td>TOTAL ACTIVE</td><td><?php echo $result;?></td></tr>
<tr><td>TOTAL QUEUED</td><td><?php echo $queued;?></td></tr>
<tr><td>ACTIVE PER VO</td><td><?php echo $line;?></td></tr>
<tr><td>AVG THROUGHPUT PER SE PAIR</td><td><?php echo $avgThr;?></td></tr>
<tr><td>AVG TX DURATION PER SE PAIR</td><td><?php echo $avgDurPerSePairxx;?></td></tr>
<tr valign="top"><td valign="top">DISTINCT FAILURES</td><td valign="top"><?php echo $reason;?></td></tr>


</table>
</body>
</html>
