@echo Importing old database snapshot.
@echo This will go horribly wrong if you've already done some of it.

@mysql -u wwiiol -pfreebird -h localhost -B <game-database.sql

@echo If there were no error messages, it's done.
