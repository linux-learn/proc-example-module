#include "demomodule.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>


///////////////////////////////////////////////////////////////////////////////
// Прототипы
//

/*
 * __init -> __attribute__((__section__(.init.text))) __attribute__((no_instrument_function))
 *
 * __attribute__ позволяет модифицировать поведение компилятора для конкретной функции/переменной.
 *
 * Атрибут __section__ позволяет положить скомпилированный код в именованную
 * секцию в итоговом объектном файле, это нужно для того что бы ядро могло
 * найти код для загрузки модуля. Подробнее про секции: https://ru.wikipedia.org/wiki/Executable_and_Linkable_Format
 *
 * При использовании профайлера в начале и в конце функции добавляются вызовы (это
 * называется инструментирование). no_instrument_function запрещает их добавлять.
 *
 * */
static int __init demomodule_init(void);
/*
 * __exit -> __attribute__((__section__(.exit.text))) __attribute__((__used__)) __attribute__((no_instrument_function))
 *
 * Атрибут __used__ позволяет сохранить переменную в объектном файле, даже если
 * на неё никто не ссылается.
 *
 */
static void __exit demomodule_exit(void);
ssize_t demomodule_read_proc(struct file *filp, char *buf, size_t count, loff_t *offp);
ssize_t demomodule_write_proc(struct file *filp, const char *buf, size_t count, loff_t *offp);


///////////////////////////////////////////////////////////////////////////////
// Локальные переменные
//
int curr = 0;
struct buffer_item *items[MAX_ITEMS] = { NULL };
struct file_operations proc_fops = {
  read: demomodule_read_proc,
  write: demomodule_write_proc
};


///////////////////////////////////////////////////////////////////////////////
// Инициализаторы
//
/*
 * По сути помещает static переменную с именем __initcall_demomodule_init в
 * секцию .initcall и .init объектного файла
 */
module_init(demomodule_init);
/*
 * По аналогии: __exitcall_demomodule_exit
 */
module_exit(demomodule_exit);


///////////////////////////////////////////////////////////////////////////////
// Реализация
//
static int __init demomodule_init(void) {
  /*
   * Посмотреть все системные сообщения можно с помощью утилиты dmesg.
   */
  printk(KERN_INFO "Hello, world!\n");

  /*
   * Добавляем виртуальный файл /proc/demomodule, API поменялось - это работает
   * начиная с 3.10, раньше надо было отдельно добавлять функцию чтения и
   * функцию записи.
   */
  if (proc_create("demomodule", 0, NULL, &proc_fops) == 0) {
    printk(KERN_ERR "Unable to register \"demomodule\" proc file\n");
    return -ENOMEM; // Out of memory
  }

  return 0;
}

static void __exit demomodule_exit(void) {
  int i;

  /*
   * Зеркальная функция для proc_create.
   */
  remove_proc_entry("demomodule", NULL);

  /*
   * Очищаем память за собой.
   */
  for (i = 0; i < MAX_ITEMS; i++) {
    if (items[i] != NULL) {
      kfree(items);
    }
  }

  printk(KERN_INFO "Goodbye, world!\n");
}

ssize_t demomodule_read_proc(struct file *filp, char *buf, size_t count, loff_t *offp) {
  struct buffer_item *item;
  size_t sz;

  if (curr <= 0) {
    return (ssize_t) 0;
  }

  item = items[--curr];

  strncpy(buf, item->buf, item->sz);

  sz = item->sz;

  kfree(item->buf);
  kfree(item);

  items[curr] = NULL;

  return (ssize_t) sz;
}

ssize_t demomodule_write_proc(struct file *filp, const char *buf, size_t count, loff_t *offp) {
  struct buffer_item *item;

  if (curr >= MAX_ITEMS) {
    return (ssize_t) 0;
  }

  item = kmalloc(sizeof(struct buffer_item), GFP_KERNEL);

  item->buf = kmalloc(sizeof(char) * count, GFP_KERNEL);
  item->sz = count;
  strncpy(item->buf, buf, count);

  items[curr++] = item;

  return (ssize_t) count;
}
