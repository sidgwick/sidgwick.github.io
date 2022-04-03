<?php

class Component
{
    private $a;
    public $b;
    protected $c;

    function __construct() {
        $this->a = "HAHA";
    }

    public function __unset($name)
    {
        $this->ounset($name);
    }

    public function ounset($name)
    {
        echo "ounset called...., name is: $name\n";
        // 这里在类内部操作了, 不会再次出发`__unset`
        unset($this->$name);
    }
}

$obj = new Component;

unset($obj->a);

var_dump(property_exists($obj, 'a'));
var_dump(property_exists($obj, 'b'));
var_dump(property_exists($obj, 'c'));
