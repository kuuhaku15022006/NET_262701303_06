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