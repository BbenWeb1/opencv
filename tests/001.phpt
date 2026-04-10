--TEST--
Check if opencv is loaded
--SKIPIF--
<?php
if (!extension_loaded('opencv')) {
    echo 'skip';
}
?>
--FILE--
<?php
echo 'The extension "opencv" is available';
?>
--EXPECT--
The extension "opencv" is available
