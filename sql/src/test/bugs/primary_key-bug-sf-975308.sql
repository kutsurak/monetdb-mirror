CREATE TABLE test_pk (
y INTEGER,
x INTEGER,
PRIMARY KEY(x, y)
);

INSERT INTO test_pk values( 2,3 );
INSERT INTO test_pk values( 2,4 );
SELECT * from test_pk;
COMMIT;

INSERT INTO test_pk values( 2,4 );
ROLLBACK;
SELECT * from test_pk;

UPDATE test_pk SET y = 1;
SELECT * from test_pk;
COMMIT;

UPDATE test_pk SET x = 1;
SELECT * from test_pk;
ROLLBACK;
SELECT * from test_pk;

DROP table test_pk;
COMMIT;
