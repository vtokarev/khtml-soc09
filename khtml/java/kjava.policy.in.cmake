grant codeBase "file:${CMAKE_INSTALL_PREFIX}/share/apps/kjava/-"
{
    permission java.security.AllPermission;
};
grant {
	permission java.AudioPermission "play";
	permission java.lang.RuntimePermission "accessClassInPackage.sun.audio";
};
