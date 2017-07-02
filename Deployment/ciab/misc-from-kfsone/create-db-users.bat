@echo Creating 'wwiiol' user
@mysql -u root -pgravytrain -h localhost -e "CREATE USER 'wwiiol'@'%%' IDENTIFIED BY 'freebird';"
@mysql -u root -pgravytrain -h localhost -e "GRANT ALL ON *.* TO 'wwiiol'@'%%';"

@echo Creating 'authuser' user
@mysql -u root -pgravytrain -h localhost -e "CREATE USER 'authuser'@'%%' IDENTIFIED BY 'scalar449';"
@mysql -u root -pgravytrain -h localhost -e "GRANT ALL ON *.* TO 'authuser'@'%%';"

@echo

@echo FINISHED:
@echo  If no errors were printed, users were created successfully.
@echo NOTE:
@echo  Users were given 'all privileges', don't do this on production.
