#!/usr/bin/env python3
import os
import re
import csv
import sys

def parse_timing_summary(summary_file):
    if not os.path.exists(summary_file):
        return {}
    with open(summary_file, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    data = {
        'wns': 'N/A',
        'tns': 'N/A',
        'whs': 'N/A',
        'ths': 'N/A',
        'clock_name': 'N/A',
        'clock_period': 'N/A',
        'clock_freq_mhz': 'N/A',
    }

    # Extract WNS / TNS
    # Format:
    # WNS(ns)      TNS(ns)  TNS Failing Endpoints ...
    #  -6.529      -28.929 ...
    m = re.search(r'WNS\(ns\)\s+TNS\(ns\)[^\n]*\n[-|\s]+\n\s*([-\d\.]+)\s+([-\d\.]+)\s+\d+\s+\d+\s+([-\d\.]+)\s+([-\d\.]+)', content)
    if m:
        data['wns'] = float(m.group(1))
        data['tns'] = float(m.group(2))
        data['whs'] = float(m.group(3))
        data['ths'] = float(m.group(4))

    # Clock summary:
    # Clock               Waveform(ns)       Period(ns)      Frequency(MHz)
    # bram_clk / clk_100_clk_gen / clk_200_clk_gen
    m_clk = re.search(r'(clk_\d+_clk_gen|clkfbout_clk_gen|ref_clk)\s+{[^}]+}\s+([\d\.]+)\s+([\d\.]+)', content)
    if m_clk:
        data['clock_name'] = m_clk.group(1)
        data['clock_period'] = float(m_clk.group(2))
        data['clock_freq_mhz'] = float(m_clk.group(3))

    return data

def parse_timing_path(path_file):
    if not os.path.exists(path_file):
        return {}
    with open(path_file, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    data = {
        'slack': 'N/A',
        'source': 'N/A',
        'destination': 'N/A',
        'data_path_delay': 'N/A',
        'logic_delay': 'N/A',
        'logic_pct': 'N/A',
        'route_delay': 'N/A',
        'route_pct': 'N/A',
        'logic_levels': 'N/A',
        'bram_cko': 'N/A',
        'bram_exit_net': 'N/A',
        'bram_total_delay': 'N/A',
        'popcount_delay': 'N/A',
        'popcount_logic': 'N/A',
        'popcount_route': 'N/A',
    }

    m_slack = re.search(r'Slack\s*(?:\([^)]+\))?\s*:\s*([-\d\.]+)ns', content)
    if m_slack:
        data['slack'] = float(m_slack.group(1))

    m_src = re.search(r'Source:\s*([^\n]+)', content)
    if m_src:
        data['source'] = m_src.group(1).strip()

    m_dst = re.search(r'Destination:\s*([^\n]+)', content)
    if m_dst:
        data['destination'] = m_dst.group(1).strip()

    # Data Path Delay: 16.435ns (logic 3.060ns (18.619%) route 13.375ns (81.381%))
    m_dp = re.search(r'Data Path Delay:\s*([\d\.]+)ns\s*\(logic\s*([\d\.]+)ns\s*\(([\d\.]+)%\)\s*route\s*([\d\.]+)ns\s*\(([\d\.]+)%\)\)', content)
    if m_dp:
        data['data_path_delay'] = float(m_dp.group(1))
        data['logic_delay'] = float(m_dp.group(2))
        data['logic_pct'] = float(m_dp.group(3))
        data['route_delay'] = float(m_dp.group(4))
        data['route_pct'] = float(m_dp.group(5))

    m_levels = re.search(r'Logic Levels:\s*(\d+)\s*\(([^)]+)\)', content)
    if m_levels:
        data['logic_levels'] = f"{m_levels.group(1)} ({m_levels.group(2)})"
    else:
        m_lev_simple = re.search(r'Logic Levels:\s*(\d+)', content)
        if m_lev_simple:
            data['logic_levels'] = m_lev_simple.group(1)

    # Detailed data path lines
    # Find the table header: Location ... Delay type ... Incr(ns) ...
    # Dashed line 1: starts launch clock path
    # Dashed line 2: starts DATA PATH
    # Dashed line 3: ends DATA PATH
    sections = re.split(r'\s*---+\s+---+\s*', content)
    if len(sections) >= 3:
        # The data path is in sections[2]
        data_path_text = sections[2]
        dp_lines = [l.strip() for l in data_path_text.strip().split('\n') if l.strip()]

        bram_delay = 0.0
        popcount_logic = 0.0
        popcount_route = 0.0
        first_cko = None
        first_exit_net = None

        for line in dp_lines:
            # Match delay increments:
            # Location (opt) Cell/Net Delay_type Incr Path Netlist_Resource
            # Examples:
            # RAMB36_X1Y1 RAMB36E1 (Prop_ramb36e1_CLKARDCLK_DOADO[0]) 0.882 -0.018 r bram_16_inst/...
            #             net (fo=1, routed)                          0.888  0.870   bram_16_inst/...
            # SLICE_X52Y9 LUT6 (Prop_lut6_I1_O)                       0.299  2.674 r test[4]_i_16/O
            #             net (fo=5, routed)                          0.687  3.361   test[4]_i_16_n_0
            m_incr = re.search(r'(\d+\.\d+)\s+[-\d\.]+\s+[rf]?\s+([\w\.\/\[\]]+)$', line)
            if not m_incr:
                continue

            incr = float(m_incr.group(1))
            res = m_incr.group(2)

            is_bram_component = ('bram_' in res.lower() or 'ramloop' in res.lower() or
                                 'prim_noinit' in res.lower() or 'psbram' in res.lower() or
                                 'douta' in res.lower())

            if is_bram_component:
                bram_delay += incr
                if first_cko is None and ('prop_' in line.lower() or 'ramb' in line.lower() or 'fdre' in line.lower()):
                    first_cko = incr
                elif first_cko is not None and first_exit_net is None and 'net' in line.lower():
                    first_exit_net = incr
            else:
                if 'prop_' in line.lower():
                    popcount_logic += incr
                elif 'net' in line.lower():
                    popcount_route += incr

        if first_cko is not None:
            data['bram_cko'] = first_cko
        if first_exit_net is not None:
            data['bram_exit_net'] = first_exit_net

        data['bram_total_delay'] = round(bram_delay, 3)
        data['popcount_logic'] = round(popcount_logic, 3)
        data['popcount_route'] = round(popcount_route, 3)
        data['popcount_delay'] = round(popcount_logic + popcount_route, 3)

    return data

def parse_utilization(util_file):
    if not os.path.exists(util_file):
        return {}
    with open(util_file, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    data = {
        'slice_luts': 'N/A',
        'slice_registers': 'N/A',
        'bram_36': 'N/A',
        'bram_18': 'N/A',
    }

    # | Slice LUTs | 350 |
    m_lut = re.search(r'\|\s*Slice LUTs\s*\|\s*(\d+)', content)
    if m_lut:
        data['slice_luts'] = int(m_lut.group(1))

    # | Slice Registers | 14 |
    m_ff = re.search(r'\|\s*Slice Registers\s*\|\s*(\d+)', content)
    if m_ff:
        data['slice_registers'] = int(m_ff.group(1))

    # | RAMB36/FIFO* | 8 | or RAMB36E1 only | 8 |
    m_b36 = re.search(r'\|\s*RAMB36(?:/FIFO\*)?\s*\|\s*(\d+)', content)
    if m_b36:
        data['bram_36'] = int(m_b36.group(1))

    m_b18 = re.search(r'\|\s*RAMB18\s*\|\s*(\d+)', content)
    if m_b18:
        data['bram_18'] = int(m_b18.group(1))

    return data

def main():
    results_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'results')
    if not os.path.exists(results_dir):
        print(f"Results directory not found: {results_dir}")
        return

    runs = sorted([d for d in os.listdir(results_dir) if d.startswith('run_')])
    if not runs:
        print("No runs found in results directory.")
        return

    rows = []

    for r in runs:
        rdir = os.path.join(results_dir, r)
        ts = parse_timing_summary(os.path.join(rdir, 'timing_summary.rpt'))
        tp_test = parse_timing_path(os.path.join(rdir, 'timing_to_test.rpt'))
        tp_pipe = parse_timing_path(os.path.join(rdir, 'timing_to_pipe.rpt'))
        tp_pipe_test = parse_timing_path(os.path.join(rdir, 'timing_pipe_to_test.rpt'))
        util = parse_utilization(os.path.join(rdir, 'utilization.rpt'))

        # Determine parameters from name: e.g. run_01_bram16_nopipe_100mhz
        m_cfg = re.match(r'run_\d+_bram(\d+)_([^_]+)_(\d+)mhz', r)
        bram_w = m_cfg.group(1) if m_cfg else '?'
        piped = m_cfg.group(2) if m_cfg else '?'
        freq = m_cfg.group(3) if m_cfg else '?'
        t_req = 10.0 if freq == '100' else 5.0

        wns = ts.get('wns', 'N/A')
        fmax = 'N/A'
        if isinstance(wns, (int, float)):
            # Fmax = 1 / (Treq - WNS)
            eff_period = t_req - wns
            if eff_period > 0:
                fmax = round(1000.0 / eff_period, 2)

        row = {
            'Run': r,
            'BRAM_Width': f"{bram_w}-bit",
            'Pipeline': piped,
            'Target_Freq_MHz': freq,
            'Target_Period_ns': t_req,
            'WNS_ns': wns,
            'Fmax_MHz': fmax,
            'Crit_Path_Delay_ns': tp_test.get('data_path_delay', 'N/A'),
            'Logic_Delay_ns': tp_test.get('logic_delay', 'N/A'),
            'Route_Delay_ns': tp_test.get('route_delay', 'N/A'),
            'Logic_Levels': tp_test.get('logic_levels', 'N/A'),
            'BRAM_Tcko_ns': tp_test.get('bram_cko', 'N/A'),
            'BRAM_Exit_Net_ns': tp_test.get('bram_exit_net', 'N/A'),
            'BRAM_Total_Delay_ns': tp_test.get('bram_total_delay', 'N/A'),
            'Popcount_Delay_ns': tp_test.get('popcount_delay', 'N/A'),
            'Popcount_Logic_ns': tp_test.get('popcount_logic', 'N/A'),
            'Popcount_Route_ns': tp_test.get('popcount_route', 'N/A'),
            'Stage1_BRAM_to_Pipe_ns': tp_pipe.get('data_path_delay', 'N/A'),
            'Stage2_Pipe_to_Test_ns': tp_pipe_test.get('data_path_delay', 'N/A'),
            'LUTs': util.get('slice_luts', 'N/A'),
            'FFs': util.get('slice_registers', 'N/A'),
            'RAMB36': util.get('bram_36', 'N/A'),
            'RAMB18': util.get('bram_18', 'N/A'),
        }
        rows.append(row)

    # Save CSV
    csv_file = os.path.join(results_dir, 'summary_results.csv')
    fieldnames = list(rows[0].keys())
    with open(csv_file, 'w', newline='', encoding='utf-8') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    print(f"Saved CSV: {csv_file}")

    # Generate Markdown Table
    md_file = os.path.join(results_dir, 'summary_results.md')
    with open(md_file, 'w', encoding='utf-8') as f:
        f.write("# Результаты исследования задержек BRAM и Popcount (12 конфигураций)\n\n")
        f.write("| № | BRAM | Pipeline | Частота | $T_{req}$ (нс) | WNS (нс) | $F_{max}$ (МГц) | Критический путь (нс) | Путь от BRAM (нс) | Сумматор Popcount (нс) | Уровни логики | LUT | FF | RAMB36/18 |\n")
        f.write("|---|------|----------|---------|----------------|----------|-----------------|----------------------|-------------------|-------------------------|---------------|-----|----|-----------|\n")
        for i, r in enumerate(rows, 1):
            bram_net = 'N/A'
            if r['BRAM_Total_Delay_ns'] != 'N/A':
                bram_net = f"{r['BRAM_Total_Delay_ns']} ns"
            popcount_str = 'N/A'
            if r['Popcount_Delay_ns'] != 'N/A':
                popcount_str = f"{r['Popcount_Delay_ns']} ns (L:{r['Popcount_Logic_ns']} + R:{r['Popcount_Route_ns']})"
            ramb = f"{r['RAMB36']}/{r['RAMB18']}"
            f.write(f"| {i} | {r['BRAM_Width']} | {r['Pipeline']} | {r['Target_Freq_MHz']} MHz | {r['Target_Period_ns']} | {r['WNS_ns']} | {r['Fmax_MHz']} | {r['Crit_Path_Delay_ns']} | {bram_net} | {popcount_str} | {r['Logic_Levels']} | {r['LUTs']} | {r['FFs']} | {ramb} |\n")

        # Also write separate table for Pipelined Stage 1 vs Stage 2
        f.write("\n## Детализация для конфигураций с конвейером (Pipeline)\n\n")
        f.write("| № | Конфигурация | Частота | Стадия 1: BRAM -> douta_pipe (нс) | Стадия 2: douta_pipe -> test (нс) |\n")
        f.write("|---|--------------|---------|------------------------------------|-----------------------------------|\n")
        pipe_rows = [r for r in rows if r['Pipeline'] == 'pipe']
        for i, r in enumerate(pipe_rows, 1):
            f.write(f"| {i} | {r['BRAM_Width']} | {r['Target_Freq_MHz']} MHz | {r['Stage1_BRAM_to_Pipe_ns']} | {r['Stage2_Pipe_to_Test_ns']} |\n")

    print(f"Saved Markdown: {md_file}")

if __name__ == '__main__':
    main()
