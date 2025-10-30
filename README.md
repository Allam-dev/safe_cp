# 🧩 safe_cp

A **safe and interactive file/directory copy tool** written in **C**, designed as a safer alternative to the Unix `cp` command.
It automatically resolves relative paths, asks before overwriting, and supports inline renaming using a simple syntax.

---

## 🚀 Features

✅&nbsp;Copy multiple files and directories in one command  
✅&nbsp;Recursively copy directories  
✅&nbsp;Handles relative paths (`.`, `..`, `./`, `../`)  
✅&nbsp;Automatically creates missing directories  
✅&nbsp;Interactive overwrite prompt  
✅&nbsp;Inline renaming with `:` syntax  
✅&nbsp;Prevents dangerous operations:  
&nbsp;&nbsp;&nbsp;&nbsp;• Copying `/`  
&nbsp;&nbsp;&nbsp;&nbsp;• Copying a directory into its child


---

## ⚙️ Usage

```bash
safe_cp -s <source1> <source2> ... -d <destination_directory>
```

### **Options**

| Option         | Description                                     |
| -------------- | ----------------------------------------------- |
| `-s`           | One or more source paths (files or directories) |
| `-d`           | Destination directory (created if missing)      |
| `-h`, `--help` | Show help message                               |

---

### **Inline Rename**

Rename a file or directory during copy:

```bash
safe_cp -s ./file.txt:newname.txt -d ./backup
```

⚠️ **Important:** The new name **must not contain `:`**.

---

### **Examples**

```bash
# Copy files to another directory
safe_cp -s ./a.txt ./b.txt -d ../backup

# Copy directories recursively
safe_cp -s ./dir1 ./dir2 -d ./copy_here

# Rename while copying
safe_cp -s ./old.txt:new.txt -d ./folder

# Copy using relative paths
safe_cp -s ../docs -d ./backup/docs
```

---

## 🧠 Internals

* Uses `realpath()`, `stat()`, and `opendir()` for filesystem operations.
* Supports reading user input interactively for overwrite confirmation.
* Uses `open()`, `read()`, `write()` for manual buffered file copying (4 KB chunks).
* Manages memory dynamically for flexible path manipulation.

---

## 🛡️ Safety

* Prevents copying `/`
* Prevents recursive copying (parent → child)
* Prompts before overwrite
* Creates missing directories safely
* Avoids overwriting directories with files or vice versa

---

## 🧑‍💻 Compilation

Compile with:

```bash
gcc safe_cp.c -o safe_cp
```

Run:

```bash
./safe_cp -s file1 dir2 -d /path/to/destination
```

---
