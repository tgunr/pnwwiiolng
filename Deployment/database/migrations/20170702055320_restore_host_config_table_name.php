<?php

use Phinx\Migration\AbstractMigration;

// Restores the wwiionline.wwii_config table to its proper location, removing the
// pointless `game` database.
class RestoreHostConfigTableName extends AbstractMigration
{
    // Because the MySQL adapter doesn't support multiple databases, can't use change()
    public function up()
    {
        $this->execute("DROP VIEW `wwiionline`.`wwii_config`");
        $this->execute("RENAME TABLE `game`.`host_config` TO `wwiionline`.`wwii_config`");
        $this->execute("DROP DATABASE `game`");
    }

    public function down()
    {
        $this->execute("CREATE DATABASE `game`");
        $this->execute("RENAME TABLE `wwiionline`.`wwii_config` TO `game`.`host_config`");
        $this->execute("CREATE SQL SECURITY INVOKER VIEW `wwiionline`.`wwii_config` AS " .
                       "SELECT * FROM `game`.`host_config`");
    }
}
