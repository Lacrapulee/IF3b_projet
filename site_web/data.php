<?php
$servername = "localhost";
$username = "root";
$password = "";
$dbname = "mesures_air";

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);

// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

$sql = "SELECT temperature, pression, altitude, humidite, co2, tvoc, date_mesure FROM mesures;";
$result = $conn->query($sql);

$data = array();
if ($result->num_rows > 0) {
    while($row = $result->fetch_assoc()) {
        $data[] = $row;
    }
} else {
    echo "0 results";
}
$conn->close();

echo json_encode($data);
?>
