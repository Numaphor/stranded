import re
import sys

def compact_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    original_lines = len(content.split('\n'))
    
    # Remove all comments
    content = re.sub(r'//.*?\n', '\n', content)
    content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
    
    # Remove all blank lines
    content = re.sub(r'\n\s*\n+', '\n', content)
    
    lines = content.split('\n')
    output = []
    i = 0
    while i < len(lines):
        line = lines[i].rstrip()
        
        # Compact if/else with single statement
        if i + 3 < len(lines):
            if (lines[i].strip().startswith('if') or lines[i].strip() == 'else') and lines[i+1].strip() == '{':
                body = lines[i+2].strip()
                if lines[i+3].strip() == '}' and len(body) < 150 and body.count('{') == 0:
                    indent = len(lines[i]) - len(lines[i].lstrip())
                    output.append(' ' * indent + lines[i].strip() + ' { ' + body + ' }')
                    i += 4
                    continue
        
        # Compact simple function bodies
        if i + 3 < len(lines) and lines[i+1].strip() == '{':
            body = lines[i+2].strip()
            if lines[i+3].strip() == '}' and len(body) < 150 and body.count('{') == 0:
                output.append(lines[i].rstrip() + ' { ' + body + ' }')
                i += 4
                continue
        
        # Compact case statements
        if i + 1 < len(lines) and lines[i].strip().startswith('case') and lines[i].strip().endswith(':'):
            next_line = lines[i+1].strip()
            if next_line and not next_line.startswith('case') and not next_line.startswith('default') and len(next_line) < 100:
                indent = len(lines[i]) - len(lines[i].lstrip())
                output.append(' ' * indent + lines[i].strip() + ' ' + next_line)
                i += 2
                continue
        
        # Merge opening brace with previous line if it's alone
        if line.strip() == '{' and output:
            output[-1] = output[-1].rstrip() + ' {'
            i += 1
            continue
        
        # Merge closing brace with previous line if previous is short
        if line.strip() == '}' and output and len(output[-1]) < 80 and output[-1].strip().endswith(';'):
            output[-1] = output[-1].rstrip() + ' }'
            i += 1
            continue
        
        if line:
            output.append(line)
        i += 1
    
    # Write result
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write('\n'.join(output))
    
    final_lines = len(output)
    saved = original_lines - final_lines
    print(f'{filepath}: {original_lines} -> {final_lines} lines (saved {saved}, {100*saved//original_lines}%)')

if __name__ == '__main__':
    files = ['src/fe_enemy.cpp', 'src/fe_npc.cpp', 'src/fe_scene.cpp', 'src/fe_game.cpp']
    for f in files:
        compact_file(f)
