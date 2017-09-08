<?php

use Phinx\Migration\AbstractMigration;

// These changes were old schema revision 2 which was never applied to the auth database
// before the new migration system (this one) was emplaced.
class TidyAuthCustomer extends AbstractMigration
{
    // Because the MySQL adapter doesn't support multiple databases, can't use change()
    public function up()
    {
        $this->execute(<<<SQL
ALTER TABLE `auth`.`auth_customer`
    MODIFY `username`  CHAR(16) CHARACTER SET latin1  NOT NULL,
    MODIFY `password`  CHAR(32) CHARACTER SET latin1  NOT NULL,
    DROP   `first`,
    DROP   `last`,
    DROP   `email`
SQL
        );
        $this->execute(<<<SQL
ALTER TABLE `auth`.`auth_subscription`
    MODIFY `description` CHAR(128) NOT NULL COMMENT 'Internal description of subscription.'
SQL
        );
    }
    
    // Note: These revert commands are imperfect, as they (of course) can not restore
    // the data to its previous state, only the table structure. However, for those
    // columns whose length was modified, it is already known that no data in these
    // columns exceeds the new, smaller limits. For those columns which were dropped,
    // it is already known that data in those columns is outdated and unmaintained.
    // In particular, billing explicitly does not sync the "first", "last", or "email"
    // fields, and has not done so for a long time.
    public function down()
    {
        $this->execute(<<<SQL
ALTER TABLE `auth`.`auth_subscription`
    MODIFY `description` char(255) NOT NULL DEFAULT '' COMMENT 'Internal description of subscription'
SQL
        );
        $this->execute(<<<SQL
ALTER TABLE `auth`.`auth_customer`
  MODIFY `username` varchar(16)  NOT NULL DEFAULT '',
  MODIFY `password` varchar(128) CHARACTER SET ascii NOT NULL DEFAULT '',
  ADD    `first`    varchar(25)  DEFAULT NULL,
  ADD    `last`     varchar(25)  DEFAULT NULL,
  ADD    `email`    varchar(128) DEFAULT NULL
SQL
        );
    }
}
