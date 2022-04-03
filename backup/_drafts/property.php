<?php
require_once('test.php');

class ttt extends Component {

}

echo "\n===============================\n";

$obj = new ttt;

unset($obj->a);

var_dump(property_exists($obj, 'a'));
var_dump(property_exists($obj, 'b'));
var_dump(property_exists($obj, 'c'));
