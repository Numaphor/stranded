# 🏠 Local GBAjs3 v4.15.1 Testing Environment

## 🎯 **Complete Local Setup - READY TO USE!**

I've successfully built a local version of GBAjs3 v4.15.1 with our `stranded.gba` ROM ready for testing!

### 📁 **Setup Location**
```
/tmp/gbajs3/gbajs3/dist/
├── index.html              # Main GBAjs3 interface
├── stranded.gba            # Our ROM with z-order fix (1.4MB)
├── assets/
│   ├── mgba-DpJTvk7N.wasm  # mGBA WASM core (1.9MB)
│   ├── index-DF8NneRY.js   # Main application (125KB)
│   └── vendor-*.js         # React, MUI, and other dependencies
└── manifest.webmanifest   # PWA manifest
```

### 🚀 **Local Server Running**
- **URL**: http://localhost:8080
- **Status**: ✅ Active (Python HTTP server)
- **ROM**: ✅ `stranded.gba` available in same directory

### 🎮 **Testing Instructions**

#### **Step 1: Access Local GBAjs3**
1. **Open Browser**: Navigate to http://localhost:8080
2. **Interface**: Full GBAjs3 v4.15.1 interface loads
3. **Features**: All features available (save states, fast forward, etc.)

#### **Step 2: Load ROM**
**Option A: Upload ROM**
1. Click "Upload Rom" in menu
2. Select `stranded.gba` from file system
3. ROM loads in mGBA WASM core

**Option B: Direct URL (Local)**
1. Navigate to: http://localhost:8080/stranded.gba
2. ROM downloads automatically
3. Upload via GBAjs3 interface

#### **Step 3: Test Z-Order Fix**
1. **Start Game**: Use Z key (A button) or virtual controls
2. **Navigate**: Use arrow keys to move player
3. **Find Merchant**: Located at position (100, -50)
4. **Test Scenarios**:
   - Move player **above** merchant → Player behind merchant ✅
   - Move player **below** merchant → Player in front ✅
5. **Debug**: Open F12 console for z-order output

### 🔧 **Build Details**

#### **Source Information**
- **Repository**: https://github.com/thenick775/gbajs3
- **Version**: v4.15.1 (tag: 4.15.1)
- **Commit**: b7b4cf8 (chore: update server deps #339)
- **Build Tool**: Vite 6.3.5
- **Framework**: React 18.2.0 + TypeScript

#### **Build Process**
```bash
# 1. Clone and checkout specific version
git clone https://github.com/thenick775/gbajs3.git
cd gbajs3 && git checkout 4.15.1

# 2. Install dependencies
cd gbajs3 && npm install

# 3. Build production version
npm run build

# 4. Serve locally
cd dist && python3 -m http.server 8080
```

#### **Build Output**
```
✓ 1254 modules transformed
✓ Built in 7.10s
✓ PWA v1.0.1 with service worker
✓ mGBA WASM core included (1.9MB)
✓ All dependencies bundled and optimized
```

### 🌟 **Advantages of Local Build**

#### **Complete Control**
- ✅ **No External Dependencies**: Fully self-contained
- ✅ **Offline Capable**: Works without internet
- ✅ **Custom Configuration**: Can modify as needed
- ✅ **Debug Access**: Full browser dev tools available

#### **Professional Features**
- ✅ **mGBA WASM Core**: Same accuracy as desktop mGBA
- ✅ **Save States**: Full save state management
- ✅ **Fast Forward**: Speed up gameplay for testing
- ✅ **Virtual Controls**: Touch-friendly interface
- ✅ **PWA Support**: Can be installed as app

#### **Testing Benefits**
- ✅ **Consistent Environment**: Same version every time
- ✅ **ROM Pre-loaded**: `stranded.gba` ready to test
- ✅ **Debug Console**: Browser F12 tools for verification
- ✅ **No Rate Limits**: Unlimited testing

### 🎯 **Z-Order Testing Workflow**

#### **Expected Behavior**
1. **Load ROM**: http://localhost:8080 → Upload `stranded.gba`
2. **Navigate**: Arrow keys to merchant at (100, -50)
3. **Test Above**: Player Y < -50 → Player behind merchant
4. **Test Below**: Player Y > -50 → Player in front of merchant
5. **Debug Output**: Console shows `Merchant z-order: -7`

#### **Success Indicators**
- ✅ **Dynamic Layering**: Depth changes based on Y position
- ✅ **Smooth Transitions**: No visual glitches
- ✅ **Debug Output**: Console shows z-order calculations
- ✅ **Consistent Behavior**: Works reliably across tests

### 🔗 **Integration with Main Project**

This local GBAjs3 build complements our main testing workflow:

1. **Primary**: Online GBAjs3 at https://gba.nicholas-vancise.dev
2. **Local**: This build at http://localhost:8080
3. **Alternative**: mGBA Docker, local mGBA installation

### 🎉 **Status: READY FOR TESTING**

The local GBAjs3 v4.15.1 environment is **fully operational** and ready to test the NPC Merchant z-order fix. This provides a controlled, offline testing environment with the exact same mGBA WASM core used by the online version.

**Perfect for comprehensive z-order verification!** 🚀

---

**Next Steps**: Open http://localhost:8080, upload `stranded.gba`, and verify the z-order fix is working correctly!
