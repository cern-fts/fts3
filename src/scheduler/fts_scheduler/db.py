import psycopg2
import psycopg2.pool


class DbConn:
    """
    Wrapper around a psycopg2 connection object that was obtained from a
    psycopg2.pool.ThreadedConnectionPool.  This wrapper automatically returns
    the connection to the pool when it is closed.
    """

    def __init__(self, pool, dbconn):
        self.open = True
        self.pool = pool
        self.dbconn = dbconn

    def close(self):
        """Returns this database connection to its pool"""
        if self.open:
            self.pool.putconn(self.dbconn)
            self.open = False

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

    def cursor(self):
        if self.open:
            return self.dbconn.cursor()
        else:
            raise Exception("Failed to get cursor from connection: Connection closed")


class DbConnPool:
    """
    Wrapper around psycopg2.pool.ThreadedConnectionPool which adds get_dbconn()
    to return DbConn objects which in turn wrap connection objects so that they
    are automatically returned to the pool when they are closed.  The
    get_dbconn() also switched autocommit on by default.
    """

    def __init__(
        self, min_conn, max_conn, host, port, db_name, user, password, sslmode
    ):
        self.pool = psycopg2.pool.ThreadedConnectionPool(
            minconn=min_conn,
            maxconn=max_conn,
            host=host,
            port=port,
            dbname=db_name,
            user=user,
            password=password,
            sslmode=sslmode,
        )

    def get_dbconn(self):
        """
        Returns a database connection from the pool with autocommit set to True
        """
        dbconn = self.pool.getconn()
        dbconn.set_session(autocommit=True)
        return DbConn(self.pool, dbconn)
