#转换代码
import torch
from models.net import U2NET

model = U2NET(3, 2)
state_dict = torch.load('net.pt', map_location='cpu')
model.load_state_dict(state_dict)  # 加载权重

model.eval()
dummy_input = torch.randn(1, 3, 416, 416)  # 调整为你模型的输入尺寸

# 方法1：trace（适用于静态模型）
traced_model = torch.jit.trace(model, dummy_input)

traced_model.save('model.pt')  # 保存
