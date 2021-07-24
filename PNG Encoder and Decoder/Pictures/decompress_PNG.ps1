$files = Get-ChildItem -Path *.png 


foreach ($file in $files)
{
	write-host $file.name
	Rename-Item -Path $file.name -NewName ($file.name + "_old.png")
	.\pngcrush.exe -force -m 1 -l 0 ($file.name + "_old.png") $file.name
}

