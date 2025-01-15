"""
Main entry point of the file-transfer scheduler daemon
"""

import argparse
import configparser
import importlib
import logging
import logging.handlers
import os
import socket
import sys
import time

from typing import Any
from db import DbConnPool
from scheduler import Scheduler


class LogException(Exception):
    """
    Logging related exception
    """


def get_log(log_file_path, log_program_name, log_level):
    """
    Creates and returns the program's logger object
    """
    hostname = socket.gethostname()

    log_format = (
        "%(asctime)s.%(msecs)03d000 "
        + hostname
        + " %(levelname)s "
        + log_program_name
        + ':LVL="%(levelname)s" PID="%(process)d" TID="%(process)d" MSG="%(message)s"'
    )
    log_date_format = "%Y/%m/%d %H:%M:%S"
    log_formatter = logging.Formatter(fmt=log_format, datefmt=log_date_format)

    log_dir = os.path.dirname(log_file_path)
    if not os.path.isdir(log_dir):
        raise LogException(
            f"Logging directory is not a directory or does not exist: path={log_dir}"
        )
    if not os.access(log_dir, os.W_OK):
        raise LogException(f"Logging directory cannot be written to: path={log_dir}")

    log_handler = logging.handlers.TimedRotatingFileHandler(
        filename=log_file_path, when="midnight", backupCount=30
    )
    log_handler.setFormatter(log_formatter)

    logger = logging.getLogger()
    logger.setLevel(log_level)
    logger.addHandler(log_handler)

    return logger


class ConfigException(Exception):
    """
    Configuration related exception
    """


def get_config(path, log_file_fallback) -> dict[str, Any]:
    """
    Parses the configuration file at the specified path and returns the result as a dict
    """
    if not os.path.isfile(path):
        print(f"Configuration file does not exist: path={path}")
        sys.exit(1)

    config_parser = configparser.ConfigParser()
    config_parser.read(path)

    if not config_parser.has_option("database", "user"):
        raise ConfigException(
            f"Missing configuration option: option=database.user path={path}"
        )

    if not config_parser.has_option("database", "password"):
        raise ConfigException(
            f"Missing configuration option: option=database.password path={path}"
        )

    if not config_parser.has_option("database", "dbname"):
        raise ConfigException(
            f"Missing configuration option: option=database.dbname path={path}"
        )

    if not config_parser.has_option("database", "host"):
        raise ConfigException(
            f"Missing configuration option: option=database.host path={path}"
        )

    if not config_parser.has_option("database", "port"):
        raise ConfigException(
            f"Missing configuration option: option=database.port path={path}"
        )

    if config_parser.has_option("log", "level"):
        valid_log_levels = ["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"]
        log_level = config_parser.get("log", "level")
        if log_level not in valid_log_levels:
            raise ConfigException(
                f"Invalid database log-level: permitted={valid_log_levels} actual={log_level}"
            )

    result = {
        "log_file": config_parser.get("log", "file", fallback=log_file_fallback),
        "log_level": config_parser.get("log", "level", fallback="INFO"),
        "sec_between_executions": config_parser.getint(
            "main", "sec_between_executions", fallback=2
        ),
        "db_user": config_parser.get("database", "user"),
        "db_password": config_parser.get("database", "password"),
        "db_dbname": config_parser.get("database", "dbname"),
        "db_host": config_parser.get("database", "host"),
        "db_port": config_parser.getint("database", "port"),
        "db_sslmode": config_parser.get("database", "sslmode", fallback="require"),
        "scheduler_algo_module_name": config_parser.get(
            "scheduler", "algo_module_name", fallback="default_scheduler_algo"
        ),
        "scheduler_algo_class_name": config_parser.get(
            "scheduler", "algo_class_name", fallback="DefaultSchedulerAlgo"
        ),
    }

    return result


def get_major_db_schema_version(param_dbconn):
    """
    Returns the major component of the database schema version
    """
    with param_dbconn.cursor() as cursor:
        cursor.execute("SELECT MAX(major) FROM  t_schema_vers")
        rows = cursor.fetchall()
        major_schema_version = rows[0][0]
        return major_schema_version


PROGRAM_NAME = "fts_scheduler"

parser = argparse.ArgumentParser(description="Schedules FTS transfer jobs")
parser.add_argument(
    "-c",
    "--config",
    default=f"/etc/fts4/{PROGRAM_NAME}.ini",
    help="Path of the configuration file",
)

cmd_line = parser.parse_args()

config = get_config(
    cmd_line.config, log_file_fallback=f"/var/log/fts4/{PROGRAM_NAME}.log"
)

log = get_log(
    log_file_path=config["log_file"],
    log_program_name=PROGRAM_NAME,
    log_level=config["log_level"],
)

log.info("Started")

NB_DBCONNS = 1
dbconn_pool = DbConnPool(
    min_conn=NB_DBCONNS,
    max_conn=NB_DBCONNS,
    host=config["db_host"],
    port=config["db_port"],
    db_name=config["db_dbname"],
    user=config["db_user"],
    password=config["db_password"],
    sslmode=config["db_sslmode"],
)

MIN_ALLOWED_MAJOR_DB_SCHEMA_VERSION = 9
with dbconn_pool.get_dbconn() as dbconn:
    actual_major_db_schema_version = get_major_db_schema_version(dbconn)
if actual_major_db_schema_version < MIN_ALLOWED_MAJOR_DB_SCHEMA_VERSION:
    print(
        f"Aborting: "
        "Database major version number is less than required: "
        f"min_allowed={MIN_ALLOWED_MAJOR_DB_SCHEMA_VERSION} "
        f"actual={actual_major_db_schema_version}"
    )
    sys.exit(1)


class ImportPythonException(Exception):
    """
    Exception relation to importing python code
    """


try:
    sys.path.append("/usr/share/fts/fts_scheduler_algo")
    try:
        scheduler_algo_module = importlib.import_module(
            config["scheduler_algo_module_name"]
        )
    except Exception as ex:
        raise ImportPythonException(f"Failed to import module: {ex}") from ex

    SchedulerAlgoClass = getattr(
        scheduler_algo_module, config["scheduler_algo_class_name"], None
    )
    if SchedulerAlgoClass is None:
        raise ImportPythonException("Failed to find scheduler-algorithm class")
except Exception as ex:
    raise ImportPythonException(
        "Failed to import the scheduler-algorithm class: "
        f"module={config['scheduler_algo_module_name']} "
        f"class={config['scheduler_algo_class_name']}: "
        f"{ex}"
    ) from ex

scheduler = Scheduler(log, SchedulerAlgoClass)

while True:
    execution_start = time.time()

    try:
        with dbconn_pool.get_dbconn() as dbconn:
            scheduler.schedule(dbconn)
    except Exception as ex:  # pylint:disable=broad-except
        log.error("Caught exception when trying to schedule: %s", str(ex))
        import traceback

        traceback.print_exc()

    execution_duration = time.time() - execution_start

    secs_to_next_execution = config["sec_between_executions"] - execution_duration
    if secs_to_next_execution > 0:
        time.sleep(secs_to_next_execution)
