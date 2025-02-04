import os
import gzip

webui_dir = 'src/webui'
print(f"Searching for HTML files in: {webui_dir}")

for root, dirs, files in os.walk(webui_dir):
    for file in files:
        if file.endswith('.html'):
            file_path = os.path.join(root, file)
            gzip_path = file_path + '.gz'
            
            print(f"Gzipping: {file_path}")
            
            with open(file_path, 'rb') as f_in:
                with gzip.open(gzip_path, 'wb') as f_out:
                    f_out.writelines(f_in)

