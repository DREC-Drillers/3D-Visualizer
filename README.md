# 3D-Visualizer

Библиотека на C++20 для визуализации трёхмерных сцен. Поддерживает треугольники,
линии и точки, перспективную и ортографическую камеры, управление мышью,
несколько областей просмотра и матовое освещение.

Реализован backend OpenGL 3.3 с окном GLFW. Библиотека экспортируется как
CMake-пакет `Visualizer::visualizer`.

## Быстрый запуск

Для Linux x86_64 нужен Nix с поддержкой `nix-command` и `flakes`.

```bash
nix develop
cmake --preset release
cmake --build --preset release
nixGLIntel ./build/release/renderTest
```

`renderTest` — интерактивный пример: десять треугольников, фоновое освещение
и два направленных источника света.

## Сборка

Все зависимости предоставляет `nix develop`.

| Preset | Назначение | Каталог |
|---|---|---|
| `release` | Библиотека и пример с оптимизацией | `build/release` |
| `debug` | Библиотека, пример и модульные тесты для отладки | `build/debug` |
| `profile` | Библиотека и пример с оптимизацией и отладочными символами | `build/profile` |
| `quality` | Библиотека, пример, тесты и compilation database для clang-tidy | `build/quality` |
| `opengl` | Интеграционные тесты отрисовки | `build/opengl` |

Например для отладки:

```bash
cmake --preset debug
cmake --build --preset debug
nixGLIntel ./build/debug/renderTest
```

## Управление примером

| Действие | Управление |
|---|---|
| Вращение камеры | ЛКМ + движение мыши |
| Перемещение камеры | ПКМ + движение мыши |
| Приближение и отдаление | Колесо мыши |
| Включить или выключить фоновый свет | `1` |
| Включить или выключить тёплый направленный свет | `2` |
| Включить или выключить холодный направленный свет | `3` |
| Выключить все источники | `0` |
| Включить все источники | `4` |
| Переключить учёт освещения материалом | `L` |
| Переключить серый и исходные цвета объектов | `C` |
| Закрыть окно | `Esc` |

## Подключение библиотеки

### Установка

После сборки установи пакет в локальный каталог:

```bash
cmake --install build/release --prefix "$PWD/build/install"
```

В каталоге появятся статические библиотеки, публичные заголовки и конфигурация
CMake-пакета. GLFW, GLM и системная поддержка потоков остаются зависимостями
потребителя; Nix-окружение проекта предоставляет их.

### CMake проекта-потребителя

```cmake
cmake_minimum_required(VERSION 3.28)
project(MyViewer LANGUAGES CXX)

find_package(Visualizer 0.1 CONFIG REQUIRED)

add_executable(my_viewer main.cpp)
set_property(TARGET my_viewer PROPERTY CXX_SCAN_FOR_MODULES OFF)
target_link_libraries(my_viewer PRIVATE Visualizer::visualizer)
```

Настрой проект, указав абсолютный путь к каталогу установки:

```bash
cmake -S /path/to/my-project -B /path/to/my-project/build -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/path/to/3D-Visualizer/build/install
cmake --build /path/to/my-project/build
```

Заголовки подключаются относительно публичного корня: например,
`<core/visualizer.hpp>` или `<scene/scene.hpp>`. Требование C++20 передаётся
потребителю через target `Visualizer::visualizer`.

### Nix-пакет

Собрать устанавливаемую библиотеку:

```bash
nix build
```

Пакет доступен через `result`. В другом flake библиотеку можно включить
в `buildInputs` как `visualizer.packages.x86_64-linux.default`, где `visualizer`
— имя flake input этого репозитория. CMake затем использует тот же
`find_package(Visualizer CONFIG REQUIRED)`.

Пакет Nix собирает только библиотеку: примеры и тесты в него не включаются.

## Использование API

### Основные объекты

| Объект | За что отвечает |
|---|---|
| `Scene` | Хранит объекты, камеры и источники света |
| `SceneObject` | Описывает геометрию одного объекта, его материал и преобразование |
| `Camera` | Определяет положение наблюдателя и проекцию |
| `ViewerBackend` | Владеет окном и рендерером, созданными `BackendFactory` |
| `Viewer` | Связывает сцены с камерами и областями окна; управляет циклом отображения |
| `OrbitCameraController` | Изменяет камеру по движению мыши и колесу |

Порядок работы: создать сцену → добавить геометрию → добавить камеру →
создать backend и `Viewer` → зарегистрировать область просмотра → запустить цикл.

### Полный минимальный пример

Это содержимое `main.cpp` для проекта-потребителя из раздела подключения.
После сборки запусти исполняемый файл через `nixGLIntel`.

```cpp
#include <cameraController/orbitCameraController.hpp>
#include <core/backendFactory.hpp>
#include <core/viewport.hpp>
#include <core/visualizer.hpp>
#include <scene/scene.hpp>

#include <memory>
#include <utility>

int main() {
    visualizer::Scene scene;

    visualizer::SceneObject triangle;
    triangle.geometry.topology = visualizer::Topology::TRIANGLES;
    triangle.geometry.vertices = {
        {{-1, -1, 0}, {0, 0, 1}},
        {{1, -1, 0}, {0, 0, 1}},
        {{0, 1, 0}, {0, 0, 1}},
    };
    triangle.material.color = {0.9f, 0.3f, 0.1f, 1.0f};
    scene.addObject(std::move(triangle));

    visualizer::Camera camera;
    camera.setPosition({0, 0, 5});
    const auto cameraId = scene.addCamera(std::move(camera));

    visualizer::BackendSettings settings;
    settings.window.title = "My scene";
    auto backend = visualizer::BackendFactory::create(settings);
    visualizer::Viewer viewer(std::move(backend));

    const visualizer::NormalizedViewport fullWindow;
    auto controller = std::make_unique<visualizer::OrbitCameraController>();
    viewer.addView(scene, cameraId, fullWindow, std::move(controller));

    viewer.run();
}
```

### Что происходит в примере

1. **`Scene scene` создаёт сцену.** В ней уже есть фоновый и направленный свет.
2. **`scene.addObject` передаёт объект сцене.** Каждая вершина содержит позицию
   и нормаль. Три вершины задают один треугольник; нормали `{0, 0, 1}` направлены
   вдоль +Z. Материал задаёт цвет и по умолчанию учитывает освещение.
3. **`scene.addCamera` сохраняет камеру и возвращает `CameraId`.** Камера
   находится на Z = 5 и при начальном повороте смотрит вдоль −Z на треугольник.
   Дальше `Viewer` обращается к камере сцены через этот идентификатор.
4. **`BackendFactory::create` создаёт окно и рендерер.** `Viewer` получает
   backend во владение через `std::move`.
5. **`viewer.addView` связывает сцену, камеру и область окна.**
   `fullWindow` с параметрами по умолчанию занимает всё окно. Контроллер
   передаётся во владение `Viewer` и обеспечивает управление камерой мышью.
6. **`viewer.run` запускает блокирующий цикл.** Он обрабатывает события,
   обновляет контроллер и рисует кадры до закрытия окна.

### Владение и время жизни

- `Scene` владеет добавленными объектами, камерами и источниками света.
- `Viewer` владеет backend и переданными контроллерами, но заимствует сцены.
- Сцена должна жить дольше использующего её `Viewer`. В примере она объявлена
  первой и уничтожается последней. Не перемещай сцену, пока `Viewer` хранит
  ссылку на неё.
- Камеру нельзя удалять из сцены, пока зарегистрированный view использует её.
- `CameraId`, `ObjectId` и `LightId` используй с той сценой или коллекцией,
  которая их создала.

[Подробный API: объекты, камеры, свет, viewport и цикл обновления](docs/api.md).

## Тестирование

Модульные тесты и проверка подключения библиотеки не требуют окна:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Тесты отрисовки запускаются отдельно:

```bash
cmake --preset opengl
cmake --build --preset opengl
visualizer-test-opengl
```

`visualizer-test-opengl` предоставляется `nix develop`. Он запускает тесты
в виртуальном дисплее с программным рендерингом Mesa. Это проверка корректности
отрисовки; для измерения производительности GPU запускай пример на нужном
графическом устройстве.

## Зависимости

Nix предоставляет Clang/LLVM 18, CMake 3.28+, Ninja, Python, GLFW, GLM,
GoogleTest, nixGL и сгенерированный GLAD для OpenGL 3.3.
Ревизии окружения закреплены в `flake.lock`.

При сборке вне Nix подготовь зависимости самостоятельно и укажи
`GLAD_SOURCE_DIR` — каталог GLAD 2 с файлами `src/gl.c` и `include/glad/gl.h`.

Примеры и тесты можно явно выключить
переменными `VISUALIZER_BUILD_EXAMPLES`, `VISUALIZER_BUILD_TESTS`
и `VISUALIZER_BUILD_OPENGL_TESTS`.
