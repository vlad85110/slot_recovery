use strict;
use warnings;
use PostgreSQL::Test::Cluster;
use PostgreSQL::Test::Utils;
use Test::More;

my $output;
my $query = 'begin;
             DO $$
             DECLARE
             i INT := 1;
             BEGIN
             WHILE i <= 655360 LOOP
                  insert into t values (i, \'vlad\');
                  i := i + 1;
             END LOOP;
             END $$;
             commit;';

my $node_primary = PostgreSQL::Test::Cluster->new('primary_1');
$node_primary->init(has_archiving => 1, allows_streaming => 1, extra => ['--wal-segsize=4']);
$node_primary->append_conf('postgresql.conf', qq(
  min_wal_size = 8MB
  max_wal_size = 16MB
  max_slot_wal_keep_size = 6
));

$node_primary->start;
$node_primary->safe_psql('postgres', 'create table t (test_a int, test_b varchar)');
$node_primary->safe_psql('postgres', "select pg_create_physical_replication_slot('slot1');");

my $backup_name = 'my_backup';
$node_primary->backup($backup_name);

my $node_standby = PostgreSQL::Test::Cluster->new('standby_1');
$node_standby->init_from_backup($node_primary, $backup_name, has_streaming => 1);
$node_standby->append_conf('postgresql.conf', "primary_slot_name = 'slot1'");
$node_standby->start;

$node_primary->wait_for_catchup($node_standby);
$node_standby->stop;
$node_primary->stop;

my $ar_dir = $node_primary->archive_dir;
$node_primary->append_conf('postgresql.conf',
    qq(shared_preload_libraries = 'slot_recovery'
       restore_command = 'test ! -f %p && cp $ar_dir/%f %p'));

$node_primary->start;
$output = $node_primary->safe_psql('postgres', 'create extension slot_recovery;');
$node_primary->safe_psql('postgres', $query);
$output = $node_primary->safe_psql('postgres', 'select wal_status from pg_replication_slots;');
is($output, 'lost', 'Check slot overflow');
$node_primary->stop;

$node_primary->start;
$node_primary->safe_psql('postgres', ' CHECKPOINT;');
$node_standby->start;
$node_primary->wait_for_log("starting recovery: oldest requested file - 000000010000000000000004; last removed requested file - 00000001000000000000000E");
$node_primary->wait_for_log("recovery complete: restore 11 files");
# todo wait for log recovery complete and started recovery
$node_primary->wait_for_catchup($node_standby);

$node_standby->stop;
$node_primary->stop;

$node_primary->start;
$node_primary->safe_psql('postgres', $query);
$output = $node_primary->safe_psql('postgres', 'select wal_status from pg_replication_slots;');
is($output, 'lost', 'Check slot overflow');
$node_primary->stop;

$node_primary->append_conf('postgresql.conf', "slot_recovery.mode = 'full'");
$node_primary->start;
$node_primary->safe_psql('postgres', ' CHECKPOINT;');
$node_standby->start;

$node_primary->wait_for_log("recovery complete: restore 12 files");
$node_primary->wait_for_log("starting recovery: oldest requested file - 00000001000000000000000F; last removed requested file - 00000001000000000000001A");
$node_primary->wait_for_catchup($node_standby);

done_testing();
