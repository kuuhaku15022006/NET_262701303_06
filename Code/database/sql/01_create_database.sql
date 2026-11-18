-- 2. Tao tai khoan ung dung
CREATE USER IF NOT EXISTS 'chat_app'@'localhost'
IDENTIFIED BY 'MatKhau103528102026!';

CREATE USER IF NOT EXISTS 'chat_app'@'127.0.0.1'
IDENTIFIED BY 'MatKhau103528102026!';

-- 3. Cap quyen cho tai khoan
GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, INDEX, REFERENCES
ON chat_tcp.*
TO 'chat_app'@'localhost';

GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, INDEX, REFERENCES
ON chat_tcp.*
TO 'chat_app'@'127.0.0.1';

-- 4. Cap nhat quyen
FLUSH PRIVILEGES;
SHOW DATABASES;
USE chat_tcp;
SHOW TABLES;
DESCRIBE users;
SELECT * FROM users;
ALTER USER 'chat_app'@'localhost'
IDENTIFIED BY 'TcpChat_DB2026!';

FLUSH PRIVILEGES;
ALTER USER 'chat_app'@'localhost'
IDENTIFIED BY 'ChatTcp_2026_New!';

ALTER USER 'chat_app'@'127.0.0.1'
IDENTIFIED BY 'ChatTcp_2026_New!';

FLUSH PRIVILEGES;
USE chat_tcp;

SHOW CREATE TABLE users;
USE chat_tcp;

ALTER TABLE users
    ADD COLUMN password_hash CHAR(64) NULL,
    ADD COLUMN password_salt CHAR(36) NULL,
    ADD COLUMN display_name VARCHAR(100) NULL,
    ADD COLUMN avatar VARCHAR(500) NULL;
    DESCRIBE users;