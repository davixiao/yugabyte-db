---
title: Restore universe data
headerTitle: Restore universe data
linkTitle: Restore universe data
description: Use YugabyteDB Anywhere to restore data.
headContent: Restore from full or incremental backups
menu:
  stable_yugabyte-platform:
    parent: back-up-restore-universes
    identifier: restore-universe-data
    weight: 30
type: docs
---

To access backups from a specific universe, navigate to the universe and choose **Backups**.

To access all universe backups, navigate to **Backups**.

## Prerequisites

- The target universe must have enough nodes to accommodate the restore.
- If the source universe is encrypted, the KMS configuration that was used to encrypt the universe. See [Back up and restore data from an encrypted at rest universe](../../security/enable-encryption-at-rest/#back-up-and-restore-data-from-an-encrypted-at-rest-universe).
- If the source backup has tablespaces, to restore the tablespaces the target universe must have a matching topology; that is, the zones and regions in the target must be the same as the zones and regions in the source.

    If the topology of the target universe does not match, none of the tablespaces are preserved and all their data is written to the primary region. After the restore, you will have to re-add all the tablespaces. For more information on specifying data placement for tables and indexes, refer to [Tablespaces](../../../explore/ysql-language-features/going-beyond-sql/tablespaces/).

## Restore an entire or incremental backup

You can restore YugabyteDB universe data from a backup as follows:

1. In the **Backups** list, select the backup to restore to display the **Backup Details**.

    ![Restore YCQL](/images/yp/restore-ycql-backup-details.png)

1. Click **Restore Entire Backup**. Alternatively, if your backup includes incremental backups, you can restore a part of an incremental backup chain by selecting an increment from the list in the **Backup Details** view and clicking its **Restore to this point**.

1. In the **Restore Backup** dialog, select the universe (target) to which you want to restore the backup.

1. If you are restoring data from a universe that has, or previously had, [encryption at rest enabled](../../security/enable-encryption-at-rest), then you must select the KMS configuration to use so that the master keys referenced in the metadata file can be retrieved.

    If the universe was previously encrypted at rest, but is not currently, then the retrieved keys assure that any existing files can be decrypted. The retrieved keys are used to build and augment the universe key registry on the restored universe with the required master keys. The universe data files are restored normally afterwards.

1. To rename databases (YSQL) or keyspaces (YCQL), select the **Rename** option.

    If you are restoring a backup to a universe with an existing databases of the same name, you must rename the database.

1. Optionally, specify the number of parallel threads that are allowed to run. This can be any number between `1` and `100`.

1. If you are restoring data from a universe that has tablespaces, select the **Restore tablespaces and data to their respective regions** option.

    To restore tablespaces, the target universe must have a topology that matches the source.

    You can restore without tablespaces, in which case none of the tablespaces are preserved and all their data is written to the primary region.

1. If you chose to rename databases or keyspaces, click **Next**, then enter new names for the databases or keyspaces that you want to rename.

1. Click **Restore**.

The restore begins immediately. When finished, a completed **Restore Backup** task appears under **Tasks > Task History**.

To confirm that the restore succeeded, select the **Tables** tab to compare the original table with the table to which you restored.

To view the details of a restored database, navigate to **Universes > your universe > Backups > Restore History**, or **Backups > Restore History**.

## Restore selected databases or keyspaces and tables

You can restore only a specific database (YSQL) or keyspace (YCQL).

In addition, if you are restoring a YCQL keyspace, you can restore only selected tables in the keyspace.

For instructions on restoring a single table in YSQL, refer to [Restore a single table in YSQL](../restore-ysql-single-table).

To restore, do the following:

1. In the **Backups** list, select the backup to restore to display the **Backup Details**.

1. In the list of databases (YSQL) or keyspaces (YCQL), click **Restore** for the database or keyspace you want to restore. If your backup includes incremental backups, to display the databases or keyspaces, click the down arrow for the increment at which you want to restore.

1. In the **Restore Backup** dialog, select the universe (target) to which you want to restore the backup.

1. If you are restoring data from a universe that has, or previously had, [encryption at rest enabled](../../security/enable-encryption-at-rest), then you must select the KMS configuration to use so that the master keys referenced in the metadata file can be retrieved.

    If the universe was previously encrypted at rest, but is not currently, then the retrieved keys assure that any existing files can be decrypted. The retrieved keys are used to build and augment the universe key registry on the restored universe with the required master keys. The universe data files are restored normally afterwards.

1. To rename databases (YSQL) or keyspaces (YCQL), select the **Rename** option.

    If you are restoring a YSQL backup to a universe with an existing database with the same name, you must rename the database.

1. If you selected a YCQL backup, you can choose to select specific tables to restore, by selecting the **Select a subset of tables** option.

    Note that this option is only available if the following conditions are met:

    - The backup was made on a universe running YugabyteDB v2.16.0 or later.
    - The selected target universe is running YugabyteDB v2.18.0 or later.

1. If you chose to rename databases/keyspaces or select tables, click **Next** to rename keyspaces and, if applicable, select tables.

1. Click **Restore**.

The restore begins immediately. When finished, a completed **Restore Backup** task appears under **Tasks > Task History**.

To confirm that the restore succeeded, select the **Tables** tab to compare the original table with the table to which you restored.

To view the details of a restored database, navigate to **Universes > Restore History**, or **Backups > Restore History**.

## Advanced restore procedure

In addition to the basic restore, an advanced option is available for when you have more than one YugabyteDB Anywhere installation and want to restore a database or keyspace from a different YugabyteDB Anywhere installation to the current universe.

### Prerequisites

To perform an advanced restore, you need the following:

- If the backup had [encryption at rest enabled](../../security/enable-encryption-at-rest), a matching KMS configuration in the target YBA installation so that the backup can be decrypted.
- A matching [storage configuration](../configure-backup-storage/) in the target YBA installation with credentials to access the storage where the backup is located.
- The storage address of the database or keyspace backup you want to restore. On the YugabyteDB Anywhere installation with the backup, do the following:

    1. In the **Backups** list, click the backup (row) to display the **Backup Details**.

    1. In the list of databases (YSQL) or keyspaces (YCQL), click **Copy Location** for the database or keyspace you want to restore. If your backup includes incremental backups, to display the databases or keyspaces, click the down arrow for the increment at which you want to restore.

    1. Note the **Storage Config** used by the backup, along with the database or keyspace name.

### Perform an advanced restore

To perform an advanced restore, on the YugabyteDB Anywhere installation where you want to perform the advanced restore, do the following:

1. On the **Backups** tab of the universe to which you want to restore, click **Advanced** and choose **Advanced Restore** to display the **Advanced Restore** dialog.

    ![Restore advanced](/images/yp/restore-advanced-ycql.png)

1. Choose the type of API.

1. In the **Backup location** field, paste the location of the backup you copied from your other installation.

1. Select the cloud provider-specific configuration of the backup storage. The storage could be on Google Cloud, Amazon S3, Azure, or Network File System.

1. Specify the name of the database or keyspace from which you are performing a restore.

1. To rename databases (YSQL) or keyspaces (YCQL) in the backup before restoring, select the rename option.

1. If the backup involved universes that had [encryption at rest enabled](../../security/enable-encryption-at-rest), then select the KMS configuration to use.

1. If you are using YBA version prior to 2.16 to manage universes with YugabyteDB version prior to 2.16, you can optionally specify the number of parallel threads that are allowed to run. This can be any number between 1 and 100.

1. If you chose to rename databases/keyspaces, click **Next**, then enter new names for the databases/keyspaces that you want to rename.

1. Click **Restore**.
