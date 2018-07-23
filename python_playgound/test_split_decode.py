import re
import urlparse

with open('inputs/body1.txt') as ifs:
    my_str = ifs.readline().rstrip()

if __name__ == '__main__':
    # test split only
    print re.split('[=&]', my_str)

    # test split and decode
    data = urlparse.parse_qsl(my_str)
    print dict(data)

    lst = ['a', 'z', 'A', 'Z', '0', '9', '-', '/', ';', '%', '=', '&']
    print zip(lst, map(hex, map(ord, lst)))

    lst = list(filter(lambda idx: my_str[idx] == '=' or my_str[idx] == '&', xrange(len(my_str))))
    lst.append(len(my_str))
    print lst

    # print hex
    print str(list(reversed(map(hex, [2 ** i - 1 for i in xrange(8)])))).replace("'", '')

    print map(len, ["interface", "method", "parameterTypesString", "parameter"])
