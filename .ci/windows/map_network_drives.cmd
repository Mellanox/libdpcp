net use M: /delete /y
net use /delete /y \\labfs01\mswg
net use M: \\labfs01\mswg /persistent:yes /user:herod herod11

net use N: /delete /y
net use /delete /y \\labfs02\windows_project
net use N: \\labfs02\windows_project /persistent:yes /user:NTYOK.mtl.com\%NFS_USER% %NFS_PASSWORD%
