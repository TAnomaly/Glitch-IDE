# Modern Text Editor

Gelişmiş özelliklerle donatılmış modern bir metin editörü. C++ ve Windows API kullanılarak geliştirilmiştir.

## 🚀 Özellikler

### ✅ Temel Özellikler
- **Çoklu Panel Desteği**: Split view ile yan yana dosya düzenleme
- **Search & Replace**: Gelişmiş arama ve değiştirme (Ctrl+F, Ctrl+H)
- **Undo/Redo**: Sınırsız geri alma ve yineleme (Ctrl+Z, Ctrl+Y)
- **Copy/Paste/Cut**: Standart clipboard işlemleri (Ctrl+C, Ctrl+V, Ctrl+X)
- **File Operations**: Dosya açma, kaydetme, yeni dosya (Ctrl+O, Ctrl+S, Ctrl+N)
- **Mouse Support**: Mouse ile metin seçimi ve panel değiştirme

### 🎨 Görsel Özellikler
- **Siyah Tema**: Göz yormayan koyu arka plan
- **Turuncu Metin**: Belirgin ve okunabilir turuncu metin rengi
- **0xNerd Proto Font**: Kalın ve modern font
- **Satır Numaraları**: Gri renkte satır numaraları
- **Status Bar**: Anlık durum bilgileri

### ⌨️ Keyboard Shortcuts

| Kısayol | Açıklama |
|---------|----------|
| `Ctrl+C` | Copy (Kopyala) |
| `Ctrl+V` | Paste (Yapıştır) |
| `Ctrl+X` | Cut (Kes) |
| `Ctrl+A` | Select All (Tümünü Seç) |
| `Ctrl+O` | Open File (Dosya Aç) |
| `Ctrl+S` | Save File (Dosya Kaydet) |
| `Ctrl+N` | New File (Yeni Dosya) |
| `Ctrl+F` | Search (Ara) |
| `Ctrl+H` | Replace (Değiştir) |
| `Ctrl+Z` | Undo (Geri Al) |
| `Ctrl+Y` | Redo (Yinele) |
| `Ctrl+1-9` | Panel Değiştir |
| `Tab` | Panel Geçişi (çoklu panel modunda) |
| `Esc` | Command Mode'a geç |

## 🏗️ Kod Yapısı

```
ModernTextEditor/
├── EditorPane.h          # Panel ve seçim yapıları
├── TextEditor.h          # Ana editör sınıfı (header)
├── TextEditor.cpp        # Ana editör sınıfı (implementation)
├── TextEditor_Part2.cpp  # Ek fonksiyonlar (TextEditor.cpp'ye ekleyin)
├── main.cpp              # Ana program ve window procedure
├── CMakeLists.txt        # CMake build dosyası
└── README.md             # Bu dosya
```

## 🔧 Derleme

### Manuel Derleme (g++)
```bash
g++ -o ModernTextEditor main.cpp TextEditor.cpp -lgdi32 -luser32 -lkernel32 -lcomdlg32
```

### CMake ile Derleme
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## 📝 Kullanım

1. **Temel Metin Düzenleme**: Insert mode'da (varsayılan) normal şekilde yazabilirsiniz
2. **Search**: `Ctrl+F` ile arama moduna geçin, aranacak metni yazın ve Enter'a basın
3. **Replace**: `Ctrl+H` ile değiştirme moduna geçin, önce aranacak sonra değiştirilecek metni yazın
4. **Split View**: Command mode'da (Esc) `:vsplit` komutu ile panel bölün
5. **Panel Geçişi**: `Ctrl+1`, `Ctrl+2` vs. ile paneller arası geçiş yapın
6. **Undo/Redo**: `Ctrl+Z` ile geri alın, `Ctrl+Y` ile yineleyin

## 🎯 Gelecek Özellikler

- [ ] Syntax Highlighting (C++, Python, JavaScript)
- [ ] Auto-completion
- [ ] Bracket matching
- [ ] Line wrapping
- [ ] Multiple cursors
- [ ] Minimap
- [ ] Code folding
- [ ] Theme system
- [ ] Plugin support
- [ ] Terminal integration
- [ ] Git integration

## 🐛 Bilinen Sorunlar

- Unicode karakter desteği sınırlı
- Çok büyük dosyalarda performans sorunları olabilir
- Font fallback sistemi yok

## 🤝 Katkıda Bulunma

1. Fork yapın
2. Feature branch oluşturun (`git checkout -b feature/amazing-feature`)
3. Değişikliklerinizi commit edin (`git commit -m 'Add amazing feature'`)
4. Branch'inizi push edin (`git push origin feature/amazing-feature`)
5. Pull Request açın

## 📄 Lisans

Bu proje MIT lisansı altında lisanslanmıştır.

## 🙏 Teşekkürler

- Windows API dokümantasyonu
- Modern C++ best practices
- Açık kaynak topluluk katkıları 