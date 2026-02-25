#!/usr/bin/php-cgi
<?php
header("Content-Type: text/plain");

echo "--- CGI PHP MULTIPART TEST ---\n";
echo "Content-Type: " . getenv('CONTENT_TYPE') . "\n";
echo "Content-Length: " . getenv('CONTENT_LENGTH') . "\n";
echo "------------------------------\n";

echo "POST DATA:\n";
print_r($_POST);

echo "\nFILES DATA:\n";
print_r($_FILES);

?>