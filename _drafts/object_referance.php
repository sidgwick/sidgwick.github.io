<?php

class foo {
    public $name;
}

function test(foo $f) {
    $f->name = "ffff";
}

$obj = new foo;
$obj->name = "Shit";

$a = $obj;
$a->name = "hah";
echo $a->name;

test($a);
echo $a->name;
