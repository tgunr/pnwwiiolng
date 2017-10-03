<?php

$clusters = [
    1 => [
        'clusterid' => 1, 'names' => ['live'], 'arenaid' => 1,
        'version' => '1.35.9.5', 'description' => "Live Campaign", 'visibility' => 'visible',
        'hosts' => [
            1 => [ 'hostname' => 'newlive1', 'command' => 'strat' ], 2 => [ 'hostname' => 'newlive2', 'command' => 'chat' ], 
            3 => [ 'hostname' => 'newlive3', 'command' => 'map' ],
        ],
		'cells' => [
			1 => [ 'hostname' => 'newlive4c1', 'activate' => true ],
			2 => [ 'hostname' => 'newlive4c2', 'activate' => true ],
			3 => [ 'hostname' => 'newlive5c1', 'activate' => false ], 
			4 => [ 'hostname' => 'newlive5c2', 'activate' => false ],
		],
    ],
    2 => [
        'clusterid' => 3, 'names' => ['Cluster3', 'Training'], 'arenaid' => 22, 
        'version' => '1.35.9.5', 'description' => "cluster 3", 'visibility' => 'visible',
        'hosts' => [
            1 => [ 'hostname' => 'cluster3-1', 'command' => 'strat' ], 2 => [ 'hostname' => 'cluster3-2', 'command' => 'chat' ],
            3 => [ 'hostname' => 'cluster3-3', 'command' => 'map' ],
        ],
		'cells' => [
			1 =>  ['hostname' => 'cluster3-4', 'activate' => true ],
		],
    ],
];

?>

