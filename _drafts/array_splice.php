<?php

$arr = array(1, 2, 3, 4);

$brr = array_splice($arr, 1, 0, array(array(6, 7)));

print_r($arr);

var_dump(array_pop($arr));

echo "\n========================\n";

$a = null;

var_dump(isset($a));
