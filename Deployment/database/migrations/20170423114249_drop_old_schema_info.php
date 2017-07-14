<?php

use Phinx\Migration\AbstractMigration;

// Remove the old database schema updater's info from the database.
class DropOldSchemaInfo extends AbstractMigration
{
    // Because the MySQL adapter doesn't support multiple databases, can't use change()
    public function up()
    {
        $this->execute("DROP TABLE IF EXISTS auth.schema_info");
    }
    
    public function down()
    {
        $this->execute(<<<SQL
CREATE TABLE `auth`.`schema_info` (
  `version_no` int(11) unsigned NOT NULL DEFAULT '0',
  `implemented` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=MyISAM
  DEFAULT CHARSET=latin1
  COMMENT='Track current database schema revision to allow automatic up'
SQL
        );
        $this->execute(<<<SQL
INSERT INTO `auth`.`schema_info` VALUES
    (0,'2010-10-13 21:22:19'),
    (1,'2010-10-13 21:22:19')
SQL
        );
    }
}
