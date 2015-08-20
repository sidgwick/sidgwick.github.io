<?php

echo date('Y-m-d 01:00:00', strtotime('now'));
echo "\n";
echo date('Y-m-d 01:00:00', strtotime('yesterday'));

echo time() - strtotime(date('Y-m-d 00:00:00'));
