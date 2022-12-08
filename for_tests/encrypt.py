import os


for path, _, files in os.walk('/home/makvv/study/sys_prog/test_folder'):
    for file_name in files:
        file_path = f'{path}/{file_name}'
        content = open(file_path, "r").read()
        salt = ord('1')
        new_content = ''
        for c in content:
            new_content += chr(ord(c) ^ salt)

        open(file_path, "w").write(new_content)
