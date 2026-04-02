# ZIP archive

This plugins allows you to create ZIP archives.

## Installation

Follow the [instructions](https://docs.halon.io/manual/comp_install.html#installation) in our manual to add our package repository and then run the below command.

### Ubuntu

```
apt-get install halon-extras-zip
```

### RHEL

```
yum install halon-extras-zip
```

## Exported classes

These classes needs to be [imported](https://docs.halon.io/hsl/structures.html#import) from the `extras://zip` module path.

### ZIP()
Create a new ZIP archive.

**Returns**: class object

```
$x = ZIP();
$x->addFile("test.txt", "hello");
$x->addFile("test2.txt", "world");
echo $x->toString(["password" => "12345678"]);
```

#### addFile(name, data)
Add a file to the ZIP archive options. On error a exception is thrown.

**Params**

- name `string` - the name
- data `string` - the data

**Returns**: this

**Return type**: `ZIP`

#### toString([options])
Return the ZIP file as a stirng. On error a exception is thrown.

**Params**

- options `array` - an options array

The following options are available in the options array.

- password `string` - If the password should be encrypted (AES256)

**Returns**: zip data

**Return type**: `string`