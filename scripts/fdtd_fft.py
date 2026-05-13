import numpy as np
import matplotlib.pyplot as plt

# ==========================================
# 1. 数据加载与 TF/SF 专属分离
# ==========================================
file_path = 'fdtd_probes_results.csv' 
data = np.loadtxt(file_path, delimiter=',', skiprows=1)
time_steps = data[:, 0]
total_steps = len(time_steps)

# 【核心改变】由于使用了工业级 TF/SF 源：
# 探针1 (idx=50) 处于散射场，它测到的数据是 100% 纯净的反射波！不需要再 split 了！
reflected_wave = data[:, 1]  

# 探针2 (idx=700) 处于总场，它测到的是纯透射波
transmitted_wave = data[:, 2] 

# 【核心新增】我们在 Python 里重构完美的解析入射脉冲
# 公式与 C++ 中的 getE_inc 严格对应 (delay=40.0, width=5.0 也就是方差25)
incident_wave = np.exp(-((time_steps - 40.0)**2) / 25.0)

# ==========================================
# 2. FFT 与 能量守恒计算
# ==========================================
freq_inc = np.abs(np.fft.fft(incident_wave))
freq_ref = np.abs(np.fft.fft(reflected_wave))
freq_trans = np.abs(np.fft.fft(transmitted_wave))

half_N = total_steps // 2

# 避开直流分量(m=0)
m_bins = np.arange(1, half_N) 
freq_inc = freq_inc[1:half_N]
freq_ref = freq_ref[1:half_N]
freq_trans = freq_trans[1:half_N]

R = (freq_ref / (freq_inc + 1e-10)) ** 2
T = (freq_trans / (freq_inc + 1e-10)) ** 2

# ==========================================
# 3. 物理量纲换算
# ==========================================
dx = 10.0 # 假设空间网格大小为 10 nm
wavelengths_nm = (total_steps * dx) / m_bins

# ==========================================
# 4. 绘制终极物理光谱图
# ==========================================
plt.style.use('default')
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
fig.patch.set_facecolor('white')

# 时域图
ax1.plot(time_steps, incident_wave, label='Ideal Incident Wave (Math)', color='blue', linestyle='--')
ax1.plot(time_steps, reflected_wave, label='Pure Reflected Wave (TF/SF)', color='red')
ax1.plot(time_steps, transmitted_wave, label='Transmitted Wave', color='green')
ax1.set_title(f'Time Domain Probes (N = {total_steps} steps)', color='black', fontweight='bold')
ax1.set_ylabel('Amplitude (Ez)')
# 放大前 2000 步看时域细节，因为 50000 步太长了，波早就跑没了
ax1.set_xlim(0, 2000) 
ax1.legend(loc='upper right')
ax1.grid(True, linestyle=':', alpha=0.6)

# 频域图
ax2.plot(wavelengths_nm, R, label='Reflectance (R)', color='red', linewidth=2)
ax2.plot(wavelengths_nm, T, label='Transmittance (T)', color='green', linewidth=2)
ax2.plot(wavelengths_nm, R + T, label='Total Energy (R+T)', color='black', linestyle=':', linewidth=2, alpha=0.8)

# 截取可见光到近红外
ax2.set_xlim(300, 1600)
ax2.set_ylim(-0.05, 1.1)

ax2.set_title('Physical Spectrum: Fabry-Perot Defect Mode', color='black', fontweight='bold')
ax2.set_xlabel('Wavelength (nm)', color='black', fontsize=12, fontweight='bold')
ax2.set_ylabel('Power Ratio (0 to 1)')
ax2.legend(loc='center right')
ax2.grid(True, linestyle='--', alpha=0.7)

plt.tight_layout()
plt.savefig('fdtd_result.png', dpi=300, bbox_inches='tight')
print("图已保存为 fdtd_result.png")