<?php

use Phinx\Migration\AbstractMigration;

class AddUserTrackingInfo extends AbstractMigration
{
    // Because the MySQL adapter doesn't support multiple databases, this doesn't work.
    /*public function change()
    {
        $connections_table = $this->table('auth.auth_connections');
        $connections_table->addColumn('mac_address', 'biginteger', ['null' => true, 'signed' => false])
                          ->addColumn('udid', 'uuid', ['null' => false])
                          ->addIndex(['mac_address'], ['name' => 'idx_mac_address'])
                          ->addIndex(['udid'], ['name' => 'idx_udid'])
                          ->addIndex(['occurred', 'ip', 'mac_address', 'udid'], ['name' => 'occurred3'])
                          ->update();
    }*/
    
    public function up()
    {
        $this->execute(<<<SQL
ALTER TABLE auth.auth_connections
    ADD COLUMN mac_address BIGINT UNSIGNED NULL DEFAULT NULL,
    ADD COLUMN udid CHAR(36) NOT NULL DEFAULT '00000000-0000-0000-0000-000000000000',
    ADD INDEX idx_mac_address (mac_address),
    ADD INDEX idx_udid (udid),
    ADD INDEX occurred3 (occurred, ip, mac_address, udid)
SQL
        );
    }
    
    public function down()
    {
        $this->execute(<<<SQL
ALTER TABLE auth.auth_connections
    DROP INDEX occurred3,
    DROP INDEX idx_udid,
    DROP INDEX idx_mac_address,
    DROP COLUMN udid,
    DROP COLUMN mac_address
SQL
        );
    }
}

?>

