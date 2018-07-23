if __name__ == '__main__':
    with open('/home/yche/workspace/comp/logs/2018-05-20_22-51/consumer/logs/consumer.log') as ifs:
        lines = filter(lambda line: 'diff time:' in line, ifs.readlines())
        lines = map(lambda line: line.rstrip().split('diff time: ')[-1].split(', endpoint number: '), lines)
        # print len(lines)
        print set(map(lambda lst: lst[1], lines[-23000:]))
        print list(filter(lambda lst: int(lst[0]) > 55000, lines[10000:]))
