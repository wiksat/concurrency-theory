from collections import deque

def get_alphabet_chars(line_strip):
    alphabet_chars = []
    for char in line_strip[3:]:
        if char.isalpha():
            alphabet_chars.append(char)
    return alphabet_chars

def parse_file(file_path):
    file_array=[]
    alphabet_array=[]
    with open(file_path, 'r') as file:
        lines = file.readlines()
    for line in lines:
        line_strip = line.strip()
        if(len(line_strip)) == 0:
            continue
        first_char = line_strip[0]
        if first_char == '(':
            alphabet_chars=get_alphabet_chars(line_strip)
            file_array.append({'name':line_strip[1],'left':alphabet_chars[0], 'right':alphabet_chars[1:]})
        elif first_char == 'A':
            alphabet_array=get_alphabet_chars(line_strip)
        elif first_char == 'w':
            word=line_strip[4:]
    return file_array, alphabet_array, word


def fillDandI(data):
    D=[]
    I=[]
    for x in range(len(data)):
        for y in range(x,len(data)):
            if data[x]['left'] in data[y]['right'] or data[y]['left'] in data[x]['right'] or data[x]['left']==data[y]['left']:
                if ((data[x]['name'],data[y]['name']) not in D): D.append((data[x]['name'],data[y]['name']))
                if ((data[y]['name'],data[x]['name']) not in D): D.append((data[y]['name'],data[x]['name']))
            else:
                if ((data[x]['name'],data[y]['name']) not in I): I.append((data[x]['name'],data[y]['name']))
                if ((data[y]['name'],data[x]['name']) not in I): I.append((data[y]['name'],data[x]['name']))
    return D,I

data, alphabet, word = parse_file('input.txt')

D,I=fillDandI(data)

print('D = ', D)
print('I = ', I)

graph = [[] for _ in range(len(word))]
for x in range(len(word)):
    for y in range(x+1, len(word)):
        if (word[x], word[y]) in D:
            graph[x].append(y)

print("Graph before optimalization:")
print(graph)

n = len(graph)
indeg = n*[0]
for vecs in graph:
	for i in vecs:
		indeg[i] += 1

indegs = [[] for i in range(n)]
for i in range(n):
	indegs[indeg[i]].append(i)


def decreaseIndegree(e):
	global indeg
	g = indeg[e]
	indeg[e] -= 1
	indegs[g].remove(e)
	indegs[g-1].append(e)

def remove(r, elems):
	global grafo, indeg
	for e in elems:
		if e in graph[r]:
			graph[r].remove(e)
			decreaseIndegree(e)

def removeRoot(r):
	global graph, indeg
	children = graph[r]
	for child in children:
		decreaseIndegree(child)

def bfs(r):
	global graph
	INF = 10000
	dist = n * [INF]
	dist[r] = 0
	queue = [r]
	while queue:
		node = queue.pop()
		remove(r, [i for i in graph[node] if dist[i]==1])
		for child in graph[node]:
			if dist[child] == INF:
				queue.append(child)
			dist[child] = min(dist[child], dist[node]+1)

while indegs[0]:
	root = indegs[0].pop()
	bfs(root)
	removeRoot(root)

print("Graph after optimalization:")
print(graph)

indeg = n*[0]
for vecs in graph:
    for i in vecs:
        indeg[i] += 1

q1 = deque()
q2 = deque()
for i in range(len(indeg)):
    if indeg[i]==0:
        q1.append(i)
fnf=""
dot_array=[]
dot_array.append("digraph G {")
while len(q1) != 0 or len(q2) != 0:
	fnf+="("
	while len(q1) != 0:
		u=q1.popleft()
		for v_id in range(len(graph)):
			if u in graph[v_id]:
				dot_array.append(str(v_id)+" -> "+str(u)+";")
		fnf+=word[u]
		for v in graph[u]:
			indeg[v]-=1
			if indeg[v]==0:
				q2.append(v)
	fnf+=")("
	while len(q2) != 0:
		u=q2.popleft()
		for v_id in range(len(graph)):
			if u in graph[v_id]:
				dot_array.append(str(v_id)+" -> "+str(u)+";")
		fnf+=word[u]
		for v in graph[u]:
			indeg[v]-=1
			if indeg[v]==0:
				q1.append(v)
	fnf+=")"

print("FNF = "+fnf)

for x in range(len(word)):
    dot_array.append(str(x)+"[label="+word[x]+"]")
dot_array.append("}")
print()
dot_array="\n".join(dot_array)
print(dot_array)

f = open("output.txt", "w")
f.write("D = "+str(D)+"\n\n")
f.write("I = "+str(I)+"\n\n")
f.write("FNF["+word+"] = "+fnf+"\n\n")
f.write(dot_array)
f.close()
