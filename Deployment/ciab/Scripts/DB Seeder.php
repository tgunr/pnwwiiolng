<?php

// --- wwii_player ---

// $conn = new mysqli('localhost', 'root', '', 'wwiionline');
// if ($conn->connect_error) {
    // die("Connection failed: " . $conn->connect_error);
// }

// for($i = 1; $i <= 100; $i++){
	// $customerId = $i+100;
	
	// $sql = "INSERT INTO `wwii_player` (`playerid`, `customerid`, `callsign`, `modified`, `added`, `lockside`, `squadid`, `locktime`, `isplaynet`, `player_decal`, `officer_unit_id`, `squad_rank`, `last_login`, `last_poll`, `squad_joined`, `last_online`, `total_time_played`, `squad_time_played`) VALUES ($i, '" . ($i+100) . "', 'crs" . $i . "', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, 0, 0, NULL, 0, 0, 0, 0, NULL, 0, NULL, NULL, NULL, NULL)";

	// if($conn->query($sql) === TRUE){
		// echo "New Area Update record created successfully for PlayerID $i CustomerID " . ($i+100) . "<br>";
	// } else {
		// echo "Error: " . $sql . "<br>" . $conn->error;
	// }
// }


// --- wwii_persona ---

// $conn = new mysqli('localhost', 'root', '', 'wwiionline');
// if ($conn->connect_error) {
    // die("Connection failed: " . $conn->connect_error);
// }

// $rank = 1;

// for($i = 2; $i <= 77; $i++){
		
	// if($i == 2 || $i == 3)
		// $rank = 25;
	
	// if($i == 4 || $i == 5)
		// $rank = 14;
	
	// if( $i == 6 || $i == 30 || $i == 54)
		// $rank = 1;
	
	// $sql = "INSERT INTO `wwii_persona` (`personaid`, `playerid`, `countryid`, `branchid`, `rankid`, `modified`, `added`, `usedmissnpts`, `rankpoints`, `brigade_id`) VALUES (NULL, $i, '1', '1', $rank, CURRENT_TIMESTAMP, NULL, '0', '0', '0'),
	// (NULL, $i, '1', '2', $rank, CURRENT_TIMESTAMP, NULL, '0', '0', '0'),
	// (NULL, $i, '1', '3', $rank, CURRENT_TIMESTAMP, NULL, '0', '0', '0'),
	// (NULL, $i, '2', '1', $rank, CURRENT_TIMESTAMP, NULL, '0', '0', '0'),
	// (NULL, $i, '2', '2', $rank, CURRENT_TIMESTAMP, NULL, '0', '0', '0'),
	// (NULL, $i, '2', '3', $rank, CURRENT_TIMESTAMP, NULL, '0', '0', '0'),
	// (NULL, $i, '3', '1', $rank, CURRENT_TIMESTAMP, NULL, '0', '0', '0'),
	// (NULL, $i, '3', '2', $rank, CURRENT_TIMESTAMP, NULL, '0', '0', '0'),
	// (NULL, $i, '3', '3', $rank, CURRENT_TIMESTAMP, NULL, '0', '0', '0'),
	// (NULL, $i, '4', '1', $rank, CURRENT_TIMESTAMP, NULL, '0', '0', '0'),
	// (NULL, $i, '4', '2', $rank, CURRENT_TIMESTAMP, NULL, '0', '0', '0'),
	// (NULL, $i, '4', '3', $rank, CURRENT_TIMESTAMP, NULL, '0', '0', '0')
	// ";

	// if($conn->query($sql) === TRUE){
		// echo "New Area Update record created successfully for PlayerID $i and Rank $rank<br>";
	// } else {
		// echo "Error: " . $sql . "<br>" . $conn->error;
	// }
	// $rank++;
// }


// --- training_progress ---

// $conn = new mysqli('localhost', 'root', '', 'wwii');
// if ($conn->connect_error) {
    // die("Connection failed: " . $conn->connect_error);
// }

// for($i = 2; $i <= 100; $i++){
	// $sql = "INSERT INTO `training_progress` (`player_id`, `group_name`, `progress`, `modified`) VALUES ($i, 'basic', '23', CURRENT_TIMESTAMP)";

	// if($conn->query($sql) === TRUE){
		// echo "New Area Update record created successfully for PlayerID $i <br>";
	// } else {
		// echo "Error: " . $sql . "<br>" . $conn->error;
	// }
// }


// --- auth_customer ---

// $conn = new mysqli('localhost', 'root', '', 'auth');
// if ($conn->connect_error) {
    // die("Connection failed: " . $conn->connect_error);
// }

// for($i = 1; $i <= 100; $i++){
	// $sql = "INSERT INTO `auth_customer` (`customerID`, `username`, `password`, `handle`, `status`, `flags`, `first`, `last`, `email`, `modified`) VALUES ('" . ($i+100) . "', 'crs" . $i . "', '5528770f4bce9c9b0ce9bbb8645aef45', 'Crs" .  $i . "', 'new', 0, NULL, NULL, NULL, CURRENT_TIMESTAMP)";

	// if($conn->query($sql) === TRUE){
		// echo "New Area Update record created successfully for PlayerID $i <br>";
	// } else {
		// echo "Error: " . $sql . "<br>" . $conn->error;
	// }
// }


// --- auth_subscribed ---

// $conn = new mysqli('localhost', 'root', '', 'auth');
// if ($conn->connect_error) {
    // die("Connection failed: " . $conn->connect_error);
// }

// $sql = "INSERT INTO `auth_subscribed` (`customerID`, `subscriptionID`, `status`) VALUES 
// ('102', 3, 'active'),
// ('102', 188, 'active'),
// ('103', 3, 'active'),
// ('103', 9, 'active'),
// ('104', 2, 'active'),
// ('104', 3, 'active'),
// ('104', 189, 'active'),
// ('105', 2, 'active'),
// ('105', 3, 'active'),
// ('105', 190, 'active')
// ";

// if($conn->query($sql) === TRUE){
	// echo "New Area Update record created successfully for Producer, CRS Staff, GM, and Trainer <br>";
// } else {
	// echo "Error: " . $sql . "<br>" . $conn->error;
// }

// for($i = 106; $i <= 177; $i++){
	
	// if($i >= 106 && $i <= 129)
		// $sub = 3;
	
	// if($i >= 130 && $i <= 153)
		// $sub = 230;
	
	// if($i >= 154 && $i <= 177)
		// $sub = 229;
	
	// $sql = "INSERT INTO `auth_subscribed` (`customerID`, `subscriptionID`, `status`) VALUES 
	// ('$i', 2, 'active'), ('$i', $sub, 'active')";

	// if($conn->query($sql) === TRUE){
		// echo "New Area Update record created successfully for PlayerID $i <br>";
	// } else {
		// echo "Error: " . $sql . "<br>" . $conn->error;
	// }
// }


// --- auth_customer ---

// $conn = new mysqli('localhost', 'root', '', 'auth');
// if ($conn->connect_error) {
    // die("Connection failed: " . $conn->connect_error);
// }

// for($i = 1; $i <= 100; $i++){
	// $sql = "INSERT INTO `wwiiol_players_t` (`playerid`, `customerid`, `added`) VALUES ('$i', '" . ($i+100) . "', CURRENT_TIMESTAMP)";

	// if($conn->query($sql) === TRUE){
		// echo "New Area Update record created successfully for PlayerID $i CustomerID " . ($i+100) . "<br>";
	// } else {
		// echo "Error: " . $sql . "<br>" . $conn->error;
	// }
// }

?>